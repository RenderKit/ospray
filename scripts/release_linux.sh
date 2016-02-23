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
export LD_LIBRARY_PATH=
#TBB_PATH_LOCAL=$PWD/tbb

# check version of symbols
function check_symbols
{
  for sym in `nm $1 | grep $2_`
  do
    version=(`echo $sym | sed 's/.*@@\(.*\)$/\1/p' | grep -E -o "[0-9]+"`)
    if [ ${#version[@]} -ne 0 ]; then
      #echo "version0 = " ${version[0]}
      #echo "version1 = " ${version[1]}
      if [ ${version[0]} -gt $3 ]; then
        echo "Error: problematic $2 symbol " $sym
        exit 1
      fi
      if [ ${version[1]} -gt $4 ]; then
        echo "Error: problematic $2 symbol " $sym
        exit 1
      fi
    fi
  done
}

mkdir -p build_release
cd build_release
# make sure to use default settings
rm -f CMakeCache.txt
rm -f ospray/version.h

# set release settings
cmake \
-D CMAKE_C_COMPILER:FILEPATH=icc \
-D CMAKE_CXX_COMPILER:FILEPATH=icpc \
-D OSPRAY_BUILD_ISA=ALL \
-D OSPRAY_BUILD_MIC_SUPPORT=ON \
-D OSPRAY_BUILD_COI_DEVICE=ON \
-D OSPRAY_BUILD_MPI_DEVICE=ON \
-D USE_IMAGE_MAGICK=OFF \
..

# read OSPRay version
OSPRAY_VERSION=`sed -n 's/#define OSPRAY_VERSION "\(.*\)"/\1/p' ospray/version.h`

# create RPM files
cmake \
-D OSPRAY_ZIP_MODE=OFF \
-D CMAKE_SKIP_INSTALL_RPATH=OFF \
-D CMAKE_INSTALL_PREFIX=/usr \
..
#-D TBB_ROOT=/usr \
make -j 16 preinstall

check_symbols libospray.so GLIBC 2 4
check_symbols libospray.so GLIBCXX 3 4
check_symbols libospray.so CXXABI 1 3
make package

# rename RPMs to have component name before version
for i in ospray-${OSPRAY_VERSION}-1.*.rpm ; do 
  newname=`echo $i | sed -e "s/ospray-\(.\+\)-\([a-z_]\+\)\.rpm/ospray-\2-\1.rpm/"`
  mv $i $newname
done

tar czf ospray-${OSPRAY_VERSION}.x86_64.rpm.tar.gz ospray-*-${OSPRAY_VERSION}-1.x86_64.rpm

# create tar.gz files
cmake \
-D OSPRAY_ZIP_MODE=ON \
-D CMAKE_SKIP_INSTALL_RPATH=ON \
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DOCDIR=doc \
-D CMAKE_INSTALL_BINDIR=bin \
..
#-D TBB_ROOT=$TBB_PATH_LOCAL \
make -j 16 preinstall

check_symbols libospray.so GLIBC 2 4
check_symbols libospray.so GLIBCXX 3 4
check_symbols libospray.so CXXABI 1 3
make package

cd ..
