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

md build
cd build

cmake --version

cmake -L `
-G $args[0] `
-T $args[1] `
-D BUILD_OSPRAY_CI_TESTS=ON `
-D BUILD_TBB_FROM_SOURCE=OFF `
-D BUILD_EMBREE_FROM_SOURCE=OFF `
-D INSTALL_IN_SEPARATE_DIRECTORIES=OFF `
../scripts/superbuild

cmake --build . --config Release --target ALL_BUILD
