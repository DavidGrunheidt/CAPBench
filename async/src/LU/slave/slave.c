/* Kernel Include */
#include <async_util.h>
#include <message.h>
#include <global.h>
#include <timer.h>
#include <util.h>
#include "slave.h"

/* C And MPPA Library Includes*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/* Data exchange segments. */
static mppa_async_segment_t matrix_segment;
static mppa_async_segment_t messages_segment;

/* Matrix width for matrix dataPut and dataGet. */
static int matrix_width;

/* Individual Slave statistics. */
uint64_t total = 0;          /* Time spent on slave.    */
uint64_t communication = 0;  /* Time spent on comms.    */               
size_t data_put = 0;         /* Number of bytes put.    */
size_t data_get = 0;         /* Number of bytes gotten. */
unsigned nput = 0;           /* Number of put ops.      */
unsigned nget = 0;           /* Number of get ops.      */

/* Timing statistics auxiliars. */
static uint64_t start, end;

/* Compute Cluster ID */
int cid;

static void cloneSegments() {
	cloneSegment(&signals_offset_seg, SIG_SEG_0, 0, 0, NULL);
	cloneSegment(&messages_segment, MSG_SEG_0, 0, 0, NULL);
	cloneSegment(&matrix_segment, MATRIX_SEG_0, 0, 0, NULL);
}

/* Applies the row reduction algorithm in a matrix. */
void row_reduction(void) {
	float mult;  /* Row multiplier. */
	float pivot; /* Pivot element.  */
	
	pivot = pvtline.elements[0];
	
	/* Apply row redution in some lines. */
	#pragma omp parallel for private(mult) default(shared)
	for (int i = 0; i < block.height; i++) {
		/* Stores the line multiplier. */
		mult = BLOCK(i, 0)/pivot;

		/* Store multiplier. */
		BLOCK(i, 0) = mult;

		/* Iterate over columns. */
		for (int j = 1; j < block.width; j++)
			BLOCK(i, j) = BLOCK(i, j) - mult*pvtline.elements[j];
	}
}

static int doWork() {
	int i0, j0;          /* Block start.              */
	struct message *msg; /* Message.                  */

	/* Waits for message io_signal to continue. */
	wait_signal();

	/* Get message from messages remote segment. */
	msg = message_get(&messages_segment, cid, NULL);
	
	switch (msg->type)	{
		/* REDUCTRESULT. */
		case REDUCTWORK :
		/* Receive pivot line. */
		dataGet(&pvtline.elements, &matrix_segment, OFFSET(matrix_width, msg->u.reductwork.ipvt, msg->u.reductwork.j0), msg->u.reductwork.width, sizeof(float), NULL);

		/* Extract message information. */
		block.height = msg->u.reductwork.height;
		block.width = pvtline.width = msg->u.reductwork.width;
		i0 = msg->u.reductwork.i0;
		j0 = msg->u.reductwork.j0;
		message_destroy(msg);

   		/* Receive matrix block. */
		dataGetSpaced(&block.elements, &matrix_segment, OFFSET(matrix_width, i0, j0)*sizeof(float), block.width*sizeof(float), block.height, (block.width+j0)*sizeof(float), NULL);

		start = timer_get();
		row_reduction();
		end = timer_get();
		total += timer_diff(start, end);

		/* Send reduct work. */
		dataPutSpaced(&block.elements, &matrix_segment, OFFSET(matrix_width, i0, j0)*sizeof(float), block.width*sizeof(float), block.height, (block.width+j0)*sizeof(float), NULL);

		/* Send reduct work done signal. */
		send_signal();

		/* Slave cant die yet. Needs to wait another message */
		return 1;

		/* DIE. */
		default:
		message_destroy(msg);
		return 0;	
	}

	mppa_rpc_barrier_all();
}

int main(__attribute__((unused))int argc, const char **argv) {
	/* Initializes async client */
	async_slave_init();

	/* Timer synchronization */
	timer_init();

	/* Util information for the problem. */
	sigback_offset = (off64_t) atoll(argv[0]);
	matrix_width = atoi(argv[1]);
	cid = __k1_get_cluster_id();

	/* Clones message exchange and io_signal segments */
	cloneSegments();

	/* Sends io_signal offset to Master. */
	send_sig_offset();

	/* Slave life. */
	while (doWork());

	/* Put statistics in stats. segment on IO side. */
	send_statistics(&messages_segment);

	/* Finalizes async library and rpc client */
	async_slave_finalize();

	return 0;
}