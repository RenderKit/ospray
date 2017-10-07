## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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

# Define an option to enable/disable this module.
#
# Though not actually required we guard each module 'moduleName' with
# a OSPRAY_MODULE_MODULENAME flag that enables/disables its building.
#
OPTION(OSPRAY_MODULE_BILINEAR_PATCH "Build pluggable geometry example module  (a Bilinear Patch)" OFF)
IF (OSPRAY_MODULE_BILINEAR_PATCH)

  # build the actual new ospray geometry type.  since this plug in
  # directly into the ospray core, we'll put it into a
  # '<module>/ospray' subdirectory.
  #
  # This subdirectory will eventually build the
  # libospray_module_<moduleName>.so library that defiens the actual
  # core geometry type.
  #
  ADD_SUBDIRECTORY(ospray)

  # build the module-specific sample apps and tools. since these are
  # stand-alone apps outside of the ospray core we'll put those into a
  # '<module>/ospray' subdirectory.
  #
  # For this module this will build the 'ospBilinearPatchViewer'
  # example. If other modules come with their own
  # tools/converters/importers/etc, this is where those sohuld go.
  #
  IF (OSPRAY_APPS_EXAMPLEVIEWER)#Must build example viewer app to build this one
    ADD_SUBDIRECTORY(apps)
  ENDIF()

ENDIF (OSPRAY_MODULE_BILINEAR_PATCH)
