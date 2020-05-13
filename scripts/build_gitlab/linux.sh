#!/bin/bash
## Copyright 2015-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

mkdir build
cd build

# NOTE(jda) - Some Linux OSs need to have lib/ on LD_LIBRARY_PATH at build time
export LD_LIBRARY_PATH=`pwd`/install/lib:${LD_LIBRARY_PATH}

cmake --version

cmake -L \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D BUILD_EMBREE_FROM_SOURCE=OFF \
  "$@" \
 ../scripts/superbuild

cmake --build .
