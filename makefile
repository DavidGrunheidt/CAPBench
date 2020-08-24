#
# Copyright(C) 2014 Pedro H. Penna <pedrohenriquepenna@gmail.com>
#

# Directories.
export BINDIR  = $(CURDIR)/bin

# Builds all kernels (using async library) for MPPA-256.
all-async:
	mkdir -p bin
	cd async && $(MAKE) all BINDIR=$(BINDIR)

# Builds all kernels (using ipc library) for MPPA-256.
all-ipc:
	mkdir -p bin
	cd ipc && $(MAKE) all BINDIR=$(BINDIR)

# Cleans async compilation files.
clean-async:
	cd async && $(MAKE) clean

# Cleans async compilation files.
clean-ipc:
	cd ipc && $(MAKE) clean
