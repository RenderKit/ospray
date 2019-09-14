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

set(OSPRAY_CMAKECONFIG_DIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/ospray-${OSPRAY_VERSION}")

set(EMBREE_VERSION_REQUIRED 3.2.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

ospray_configure_build_type()
ospray_configure_compiler()

###########################################################
# OSPRay's dependencies
###########################################################

# ospcommon
find_package(ospcommon REQUIRED)

# embree
ospray_find_embree(${EMBREE_VERSION_REQUIRED})
ospray_verify_embree_features()
ospray_determine_embree_isa_support()
ospray_create_embree_target()

###########################################################
# OSPRay specific build options and configuration selection
###########################################################

# Configure OSPRay ISA last after we've detected what we got w/ Embree
ospray_configure_ispc_isa()

option(OSPRAY_ENABLE_APPS "Enable the 'apps' subtree in the build." ON)

option(OSPRAY_ENABLE_MODULES "Enable the 'modules' subtree in the build." ON)
mark_as_advanced(OSPRAY_ENABLE_MODULES)

option(OSPRAY_ENABLE_TESTING
       "Enable building, installing, and packaging of test tools.")

option(OSPRAY_ENABLE_TARGET_CLANGFORMAT
       "Enable 'format' target, requires clang-format too")
mark_as_advanced(OSPRAY_ENABLE_TARGET_CLANGFORMAT)

#####################################################################
# Binary package options, before any install() invocation/definition
#####################################################################

option(OSPRAY_ZIP_MODE "Use tarball/zip CPack generator instead of RPM" ON)
mark_as_advanced(OSPRAY_ZIP_MODE)

option(OSPRAY_INSTALL_DEPENDENCIES
       "Install OSPRay dependencies in binary packages and install")
mark_as_advanced(OSPRAY_INSTALL_DEPENDENCIES)
