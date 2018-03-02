## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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

# this subdirectory builds the actual ospray plugin into a library
# "libospray_module_<modulename>.so".
#
# This shared library can depend on libospray, but should _not_ assume
# that it is living in the same address space as the application: When
# running in MPI mode ospray will, upon encountering the
# ospLoadModule("<moduleName>"), actually load and use this library on
# every worker node (which will typically not run the application). As
# such, the library should _not_ "commuincate" with the application
# through global variables or globally defind functions, and instead
# communicate solely through the ospNewGeometry(...), ospSet...(),
# ospCommit(...) etc functionality provided by ospray.


# build the actual shared library that defines the geometry.
#
# Note the name is important: In order for ospray to properly find and
# initialize a module referenced by a call to
# "ospLoadModule(<moduleName>) this module _has_ to
#
# a) be called libospray_module_<modulename>.so, and
# b) contain a (extern C linkage) initializatoin routine named
#    void ospray_init_module_<moduleName>()
#
OSPRAY_CREATE_LIBRARY(ospray_module_bilinear_patches
  # the cpp file that contains all the plugin code - parsing
  # parameters in ospCommit(), creating and registering the object,
  # building accel strcutures, etc
  geometry/BilinearPatches.cpp

  # the ispc file that contains the plugins for all vector code - ie,
  # for ray-primitive intersection and 'postIntersect' (reporting info
  # on a previously computed ray-prim intersection)
  geometry/BilinearPatches.ispc

  # pieces for loading data into the OSPRay example viewer app
#  sg/Patch.cpp

  # and finally, the module init code (not doing much, but must be there)
  moduleInit.cpp

  # this depends on ospray core:
  LINK
  ospray_module_ispc
)
