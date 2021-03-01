#!/bin/sh
# StarPU --- Runtime system for heterogeneous multicore architectures.
#
# Copyright (C) 2016-2021  Université de Bordeaux, CNRS (LaBRI UMR 5800), Inria
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
#
. $(dirname $0)/microbench.sh

XFAIL="heteroprio"

if [ -z "$STARPU_BENCH_DIR" ]
then
	FAST="-i 8"
fi

_STARPU_LAUNCH="$STARPU_LAUNCH"
unset STARPU_LAUNCH
test_scheds tasks_size_overhead_sched.sh $FAST
