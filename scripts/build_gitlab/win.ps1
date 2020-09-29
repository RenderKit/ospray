## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

md build
cd build

cmake --version

cmake -L `
  -G $args[0] `
  -T $args[1] `
  -D BUILD_EMBREE_FROM_SOURCE=OFF `
  -D CMAKE_BUILD_TYPE=${args[2]} `
  -D DEPENDENCIES_BUILD_TYPE=${args[2]} `
  ../scripts/superbuild

cmake --build . --config $args[2] --target ALL_BUILD

exit $LASTEXITCODE
