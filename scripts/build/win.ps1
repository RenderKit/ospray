## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

Param(
  [string] $G = 'Ninja',
  [ValidateSet('Release', 'RelWithDebInfo', 'Debug')][string] $buildType = 'Release'
)

md build
cd build

cmake --version

$exitCode = 0

cmake -L `
  -G $G `
  $args `
  -D CMAKE_BUILD_TYPE=$buildType `
  -D DEPENDENCIES_BUILD_TYPE=$buildType `
  ../scripts/superbuild
if ($LastExitCode) { $exitCode++ }

cmake --build . --config $buildType
if ($LastExitCode) { $exitCode++ }

exit $exitCode
