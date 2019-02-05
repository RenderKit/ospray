#!/bin/bash
## ======================================================================== ##
## Copyright 2015-2019 Intel Corporation                                    ##
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

mkdir build
cd build

cmake \
  -D OSPRAY_BUILD_ISA=ALL \
  -D OSPRAY_ENABLE_TESTING=ON \
  -D OSPRAY_AUTO_DOWNLOAD_TEST_IMAGES=OFF \
  -D OSPRAY_MODULE_BILINEAR_PATCH=ON \
  -D OSPRAY_SG_CHOMBO=OFF \
  -D OSPRAY_SG_OPENIMAGEIO=OFF \
  -D OSPRAY_SG_VTK=OFF \
  "$@" ..

make -j`nproc` && make test
