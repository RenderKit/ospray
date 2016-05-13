## ======================================================================== ##
## Copyright 2015-2016 Intel Corporation                                    ##
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

#!/bin/sh

mkdir build
cd build

cmake -L \
-G "Visual Studio 12 2013 Win64" \
-T "Intel C++ Compiler 16.0" \
-D OSPRAY_BUILD_ISA=ALL \
-D OSPRAY_BUILD_MIC_SUPPORT=OFF \
-D USE_IMAGE_MAGICK=OFF \
..

cmake --build . --config Release --target PACKAGE -- -m -nologo
