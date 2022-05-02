## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

$ROOT_DIR = pwd
$DEP_DIR = "$ROOT_DIR\deps"

## Build dependencies ##

mkdir deps_build
cd deps_build

cmake --version

cmake -L `
  -G $($args[0]) `
  -D BUILD_DEPENDENCIES_ONLY=ON `
  -D CMAKE_INSTALL_PREFIX=$DEP_DIR `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D BUILD_EMBREE_FROM_SOURCE=OFF `
  -D BUILD_OIDN=ON `
  -D BUILD_OIDN_FROM_SOURCE=OFF `
  -D BUILD_OSPRAY_MODULE_MPI=ON `
  -D INSTALL_IN_SEPARATE_DIRECTORIES=OFF `
  ../scripts/superbuild

cmake --build . --config Release --target ALL_BUILD

cd $ROOT_DIR

#### Build OSPRay ####

md build_release
cd build_release

# Setup environment for dependencies
$env:CMAKE_PREFIX_PATH = $DEP_DIR

# set release settings
cmake -L `
  -G $($args[0]) `
  -D CMAKE_PREFIX_PATH="$DEP_DIR\lib\cmake" `
  -D OSPRAY_BUILD_ISA=ALL `
  -D ISPC_EXECUTABLE=$DEP_DIR/bin/ispc.exe `
  -D TBB_ROOT=$DEP_DIR `
  -D OSPRAY_ZIP_MODE=OFF `
  -D OSPRAY_MODULE_DENOISER=ON `
  -D OSPRAY_INSTALL_DEPENDENCIES=ON `
  -D USE_STATIC_RUNTIME=OFF `
  -D CMAKE_INSTALL_INCLUDEDIR=include `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D CMAKE_INSTALL_DATAROOTDIR= `
  -D CMAKE_INSTALL_DOCDIR=doc `
  -D CMAKE_INSTALL_BINDIR=bin `
  -D CMAKE_BUILD_WITH_INSTALL_RPATH=bin `
  -D OSPRAY_MODULE_MPI=ON `
  -D OSPRAY_SIGN_FILE=$env:SIGN_FILE_WINDOWS `
  ..

# compile and create installers
# option '--clean-first' somehow conflicts with options after '--' for msbuild
cmake --build . --config Release --target sign_files
cmake --build . --config Release --target PACKAGE

# create ZIP files
cmake -L `
  -D OSPRAY_ZIP_MODE=ON `
  ..

cmake --build . --config Release --target PACKAGE

exit $LASTEXITCODE
