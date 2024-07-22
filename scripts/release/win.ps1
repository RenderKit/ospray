## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

Param(
  [string] $G = 'Ninja'
)

$ROOT_DIR = pwd
$DEP_DIR = "$ROOT_DIR\deps"

## Build dependencies ##

mkdir deps_build
cd deps_build

cmake --version

$exitCode = 0

cmake -L `
  -G $G `
  $args `
  -D CMAKE_BUILD_TYPE=Release `
  -D DEPENDENCIES_BUILD_TYPE=Release `
  -D BUILD_DEPENDENCIES_ONLY=ON `
  -D CMAKE_INSTALL_PREFIX=$DEP_DIR `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D BUILD_OSPRAY_MODULE_MPI=ON `
  -D BUILD_GPU_SUPPORT=ON `
  -D INSTALL_IN_SEPARATE_DIRECTORIES=OFF `
  ../scripts/superbuild
if ($LastExitCode) { $exitCode++ }

cmake --build . --config Release
if ($LastExitCode) { $exitCode++ }

cd $ROOT_DIR

#### Build OSPRay ####

md build_release
cd build_release

# Clean out build directory to be sure we are doing a fresh build
rm -r -fo *

# Setup environment for dependencies
$env:CMAKE_PREFIX_PATH = $DEP_DIR

if ($G -eq 'Ninja') {
  $package = 'package'
  # FIXME WA for OSPRay to build with GNU-style options
  $cxx_compiler = '-DCMAKE_CXX_COMPILER=clang++'
  $c_compiler = '-DCMAKE_C_COMPILER=clang'
} else {
  $package = 'PACKAGE'
}

# set release settings
cmake -L `
  -G $G `
  $args `
  $cxx_compiler `
  $c_compiler `
  -D CMAKE_BUILD_TYPE=Release `
  -D USE_STATIC_RUNTIME=OFF `
  -D CMAKE_PREFIX_PATH="$DEP_DIR\lib\cmake" `
  -D OSPRAY_MODULE_PATH="$ROOT_DIR\deps_build" `
  -D TBB_ROOT=$DEP_DIR `
  -D OSPRAY_ZIP_MODE=OFF `
  -D OSPRAY_BUILD_ISA=ALL `
  -D OSPRAY_INSTALL_DEPENDENCIES=ON `
  -D OSPRAY_MODULE_DENOISER=ON `
  -D OSPRAY_MODULE_MPI=ON `
  -D OSPRAY_MODULE_GPU=ON `
  -D CMAKE_INSTALL_INCLUDEDIR=include `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D CMAKE_INSTALL_DOCDIR=doc `
  -D CMAKE_INSTALL_BINDIR=bin `
  -D CMAKE_INSTALL_DATAROOTDIR= `
  -D OSPRAY_SIGN_FILE=$env:SIGN_FILE_WINDOWS `
  ..
if ($LastExitCode) { $exitCode++ }

# compile and create installers
cmake --build . --config Release --target sign_files
if ($LastExitCode) { $exitCode++ }

cmake --build . --config Release --target $package
if ($LastExitCode) { $exitCode++ }

# create ZIP files
cmake -L `
  -D OSPRAY_ZIP_MODE=ON `
  ..
if ($LastExitCode) { $exitCode++ }

cmake --build . --config Release --target $package
if ($LastExitCode) { $exitCode++ }

exit $exitCode
