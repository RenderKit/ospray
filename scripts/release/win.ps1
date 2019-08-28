## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

$ROOT_DIR = pwd
$DEP_DIR = "$ROOT_DIR\deps"

## Build dependencies ##

mkdir deps_build
cd deps_build

cmake --version

cmake -L `
  -G $args[0] `
  -D BUILD_DEPENDENCIES_ONLY=ON `
  -D CMAKE_INSTALL_PREFIX=$DEP_DIR `
  -D CMAKE_INSTALL_LIBDIR=lib `
  -D BUILD_TBB_FROM_SOURCE=OFF `
  -D BUILD_EMBREE_FROM_SOURCE=OFF `
  -D INSTALL_IN_SEPARATE_DIRECTORIES=OFF `
  ../scripts/superbuild

cmake --build . --config Release --target ALL_BUILD

cd ..

## Build OSPRay ##

md build_release
cd build_release

# set release settings
cmake -L `
-G $args[0] `
-D CMAKE_PREFIX_PATH="$DEP_DIR\lib\cmake" `
-D OSPRAY_BUILD_ISA=ALL `
-D OSPRAY_ZIP_MODE=OFF `
-D OSPRAY_INSTALL_DEPENDENCIES=ON `
-D USE_STATIC_RUNTIME=OFF `
-D CMAKE_INSTALL_INCLUDEDIR=include `
-D CMAKE_INSTALL_LIBDIR=lib `
-D CMAKE_INSTALL_DATAROOTDIR= `
-D CMAKE_INSTALL_DOCDIR=doc `
-D CMAKE_INSTALL_BINDIR=bin `
..

# compile and create installers
# option '--clean-first' somehow conflicts with options after '--' for msbuild
cmake --build . --config Release --target PACKAGE

# create ZIP files
cmake -D OSPRAY_ZIP_MODE=ON `
-D OSPRAY_APPS_ENABLE_DENOISER=ON `
-D OSPRAY_INSTALL_DEPENDENCIES=ON `
..

cmake --build . --config Release --target PACKAGE

cd ..
