#!/bin/bash
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

source $srcdir/microbenchs/microbench.sh

XFAIL="modular-eager-prefetching modular-prio-prefetching modular-random-prefetching modular-random-prio-prefetching modular-ws modular-heft modular-heft2 random peager heteroprio graph_test"

test_scheds parallel_independent_heterogeneous_tasks
