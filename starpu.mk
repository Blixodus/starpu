# StarPU --- Runtime system for heterogeneous multicore architectures.
#
# Copyright (C) 2016  Université de Bordeaux
#
# StarPU is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or (at
# your option) any later version.
#
# StarPU is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# See the GNU Lesser General Public License in COPYING.LGPL for more details.


if STARPU_USE_MPI_MASTER_SLAVE
MPI_LAUNCHER 			= $(MPIEXEC)  $(MPIEXEC_ARGS) -np 4
MPI_RUN_ARGS			= STARPU_WORKERS_NOBIND=1 STARPU_NCPU=4 STARPU_NMPIMSTHREADS=4
endif

showcheck:
	-cat $(TEST_LOGS) /dev/null
	@! grep -q "ERROR: AddressSanitizer: " $(TEST_LOGS) /dev/null
	@! grep -q "WARNING: AddressSanitizer: " $(TEST_LOGS) /dev/null
	@! grep -q "ERROR: ThreadSanitizer: " $(TEST_LOGS) /dev/null
	@! grep -q "WARNING: ThreadSanitizer: " $(TEST_LOGS) /dev/null
	@! grep -q "ERROR: LeakSanitizer: " $(TEST_LOGS) /dev/null
	@! grep -q "WARNING: LeakSanitizer: " $(TEST_LOGS) /dev/null
	@! grep -q " runtime error: " $(TEST_LOGS) /dev/null
	RET=0 ; \
	for i in $(SUBDIRS) ; do \
		make -C $$i showcheck || RET=1 ; \
	done ; \
	exit $$RET
