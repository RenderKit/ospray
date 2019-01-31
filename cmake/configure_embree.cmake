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


# Find existing Embree on the machine #######################################

ospray_find_embree(${EMBREE_VERSION_REQUIRED})
ospray_verify_embree_features()
ospray_determine_embree_isa_support()
ospray_create_embree_target()
ospray_install_library(embree lib)

# Configure OSPRay ISA last after we've detected what we got w/ Embree
ospray_configure_ispc_isa()
