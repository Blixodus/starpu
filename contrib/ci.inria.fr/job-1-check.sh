#!/bin/sh
# StarPU --- Runtime system for heterogeneous multicore architectures.
#
# Copyright (C) 2013-2018                                CNRS
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

set -e
set -x

export PKG_CONFIG_PATH=/home/ci/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/home/ci/usr/local/lib:$LD_LIBRARY_PATH

tarball=$(ls -tr starpu-*.tar.gz | tail -1)

if test -z "$tarball"
then
    echo Error. No tar.gz file
    ls
    pwd
    exit 1
fi

basename=$(basename $tarball .tar.gz)
export STARPU_HOME=$PWD/$basename/home
mkdir -p $basename
cd $basename
env > $PWD/env

test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
tar xfz ../$tarball
cd $basename
mkdir build
cd build

STARPU_CONFIGURE_OPTIONS=""
suname=$(uname)
if test "$suname" == "Darwin"
then
    STARPU_CONFIGURE_OPTIONS="--without-hwloc"
fi
if test "$suname" == "OpenBSD"
then
    STARPU_CONFIGURE_OPTIONS="--without-hwloc --disable-mlr"
fi

export CC=gcc

day=$(date +%u)
if test $day -le 5
then
    ../configure --enable-quick-check --enable-verbose --enable-mpi-check --disable-build-doc $STARPU_CONFIGURE_OPTIONS
else
    ../configure --enable-long-check --enable-verbose --enable-mpi-check --disable-build-doc $STARPU_CONFIGURE_OPTIONS
fi

make
#make check
(make -k check || true) > ../check_$$ 2>&1
cat ../check_$$
make showcheck

grep "^FAIL:" ../check_$$ || true

make clean

grep "^FAIL:" ../check_$$ || true

echo "Running on $(uname -a)"
exit $(grep "^FAIL:" ../check_$$ | wc -l)

