#!/usr/bin/env bash
## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -ex

mkdir build
cd build

cmake --version

cmake -L \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D BUILD_EMBREE_FROM_SOURCE=ON \
  "$@" \
 ../scripts/superbuild

cmake --build .
