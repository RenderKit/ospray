## ======================================================================== ##
## Copyright 2015-2017 Intel Corporation                                    ##
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

#!/bin/bash

# to make sure we do not include nor link against wrong TBB
export CPATH=
export LIBRARY_PATH=
export LD_LIBRARY_PATH=

mkdir -p build_release
cd build_release

# fetch docu pdf (in correct version)
BRANCH=$CI_BUILD_REF_NAME
if [ -z $BRANCH ]; then
  BRANCH=`git rev-parse --abbrev-ref HEAD`
fi
wget -O readme.pdf --progress=dot:mega -c http://sdvis.org/ospray/download/OSPRay_readme_$BRANCH.pdf

# set release settings
cmake -L \
-G "Visual Studio 12 2013 Win64" \
-T "Intel C++ Compiler 17.0" \
-D OSPRAY_ZIP_MODE=OFF \
-D OSPRAY_BUILD_ISA=ALL \
-D OSPRAY_USE_EXTERNAL_EMBREE=ON \
-D USE_IMAGE_MAGICK=OFF \
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DATAROOTDIR= \
-D CMAKE_INSTALL_DOCDIR=doc \
-D CMAKE_INSTALL_BINDIR=bin \
..

# compile and create installers
# option '--clean-first' somehow conflicts with options after '--' for msbuild
cmake --build . --config Release --target PACKAGE -- -m -nologo

# create ZIP files
cmake -D OSPRAY_ZIP_MODE=ON ..
cmake --build . --config Release --target PACKAGE -- -m -nologo

cd ..
