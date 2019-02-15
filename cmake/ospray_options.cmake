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

##############################################################
# Global configuration options
##############################################################

include(GNUInstallDirs)

set(OSPRAY_CMAKECONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/ospray-${OSPRAY_VERSION}")

set(EMBREE_VERSION_REQUIRED 3.2.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

if (WIN32)
  # avoid problematic min/max defines of windows.h
  add_definitions(-DNOMINMAX)
endif()

###########################################################
# OSPRay specific build options and configuration selection
###########################################################

ospray_configure_build_type()
ospray_configure_compiler()
ospray_configure_tasking_system()
ospray_create_tasking_target()

# Must be before ISA config and package
include(configure_embree)

option(OSPRAY_ENABLE_APPS "Enable the 'apps' subtree in the build." ON)

option(OSPRAY_ENABLE_MODULES "Enable the 'modules' subtree in the build." ON)
mark_as_advanced(OSPRAY_ENABLE_MODULES)

option(OSPRAY_ENABLE_TESTING
       "Enable building, installing, and packaging of test tools.")
if (OSPRAY_ENABLE_TESTING)
  enable_testing()
  option(OSPRAY_AUTO_DOWNLOAD_TEST_IMAGES
         "Automatically download test images during build." ON)
endif()

option(OSPRAY_ENABLE_TARGET_CLANGFORMAT
       "Enable 'format' target, requires clang-format too")
mark_as_advanced(OSPRAY_ENABLE_TARGET_CLANGFORMAT)

####################################################################
# Create binary packages; before any install() invocation/definition
####################################################################

option(OSPRAY_ZIP_MODE "Use tarball/zip CPack generator instead of RPM" ON)
mark_as_advanced(OSPRAY_ZIP_MODE)

option(OSPRAY_INSTALL_DEPENDENCIES
       "Install OSPRay dependencies in binary packages and install")
mark_as_advanced(OSPRAY_INSTALL_DEPENDENCIES)

include(package)
