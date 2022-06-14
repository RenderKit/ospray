#!/bin/bash
## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

mkdir build
cd build

cmake --version

cmake -L \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D BUILD_EMBREE_FROM_SOURCE=OFF \
  "$@" \
 ../scripts/superbuild

cmake --build .
