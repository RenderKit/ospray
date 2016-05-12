## ======================================================================== ##
## Copyright 2014-2016 Intel Corporation                                    ##
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
export DYLD_LIBRARY_PATH=
TBB_PATH_LOCAL=$PWD/tbb

umask=`umask`
function onexit {
  umask $umask
}
trap onexit EXIT
umask 002

mkdir -p build_release
cd build_release
# make sure to use default settings
rm -f CMakeCache.txt
rm -f ospray/version.h

# set release and installer settings
cmake \
-D CMAKE_C_COMPILER:FILEPATH=icc \
-D CMAKE_CXX_COMPILER:FILEPATH=icpc \
-D OSPRAY_BUILD_ISA=ALL \
-D OSPRAY_BUILD_MIC_SUPPORT=ON \
-D OSPRAY_BUILD_COI_DEVICE=ON \
-D OSPRAY_BUILD_MPI_DEVICE=ON \
-D USE_IMAGE_MAGICK=OFF \
-D OSPRAY_ZIP_MODE=OFF \
-D CMAKE_INSTALL_PREFIX=/opt/local \
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DOCDIR=../../Applications/OSPRay/doc \
-D CMAKE_INSTALL_BINDIR=../../Applications/OSPRay/bin \
..
#-D TBB_ROOT=/opt/local \

# create installers
make -j 4 package

# change settings for zip mode
cmake \
-D OSPRAY_ZIP_MODE=ON \
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DOCDIR=doc \
-D CMAKE_INSTALL_BINDIR=bin \
-D TBB_ROOT=$TBB_PATH_LOCAL \
..

# create ZIP files
make -j 4 package

cd ..
