## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

md build
cd build

cmake --version

cmake -L `
  -G $args[0] `
  -T $args[1] `
  -D BUILD_EMBREE_FROM_SOURCE=OFF `
  -D BUILD_OIDN=ON `
  -D INSTALL_IN_SEPARATE_DIRECTORIES=OFF `
  ../scripts/superbuild

cmake --build . --config Release --target ALL_BUILD

exit $LASTEXITCODE
