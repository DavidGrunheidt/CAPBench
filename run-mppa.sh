#
# Copyright(C) 2014 Pedro H. Penna <pedrohenriquepenna@gmail.com>
#

# rsync -avzh mppa:/home/lig-ext/ordine/results ./results
# rsync -avzh ./ mppa:/home/lig-ext/ordine/capbench

# Directories.
export BINDIR=bin
export K1DIR=/usr/local/k1tools/bin
export RESULTSDIR=$PWD/../results

#KERNEL=gf fast lu is km fn

# km large -> 2 4 8 16 || huge -> 4 8 16

for KERNEL in fn; do
	for CLASS in standard; do
		for NPROCS in 2 4 8 16; do
			for repeat in 1; do

				# Create result directory.
				output_directory=$RESULTSDIR/$KERNEL/$CLASS/$NPROCS
				mkdir -p $output_directory

				echo "  ========== Running $KERNEL kernel in $CLASS class with $NPROCS clusters ($repeat)"
				$K1DIR/k1-power 										\
					--traces_keep --gnuplot=pdf --period=20 			\
					--output=$output_directory/power-$repeat  			\
					-- $K1DIR/k1-jtag-runner                			\
					--multibinary=$BINDIR/$KERNEL.img					\
					--exec-multibin=IODDR0:io_bin            		  	\
					-- --verbose --class $CLASS --nclusters $NPROCS   	\
					&>> $output_directory/time-$repeat
			done
		done
	done
done