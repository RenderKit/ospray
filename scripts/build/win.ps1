## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

  
Param(
  [string] $G = 'Ninja',
  [ValidateSet('Release', 'RelWithDebInfo', 'Debug')][string] $buildType = 'Release'
)

md build
cd build

cmake --version

cmake -L `
  -G $G `
  $args `
  -D CMAKE_BUILD_TYPE=$buildType `
  -D DEPENDENCIES_BUILD_TYPE=$buildType `
  ../scripts/superbuild

if ($G -ne 'Ninja') {
  $target = '--target ALL_BUILD'
}

cmake --build . --config $buildType $target

exit $LASTEXITCODE
