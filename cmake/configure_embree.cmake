## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
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

SET(OSPRAY_ALLOW_EXTERNAL_EMBREE OFF CACHE BOOL "Allow building ospray w/ EXTERNAL embree")
MARK_AS_ADVANCED(OSPRAY_ALLOW_EXTERNAL_EMBREE)

IF (OSPRAY_ALLOW_EXTERNAL_EMBREE)
  ##################################################################
  # if you are using this branch we assume you know what you are doing!
  ##################################################################
  MESSAGE("You have chosen to build OSPRay with a external version of embree that may or may not have been tested to work with this version of OSPRay. Use this option at your own risk.")
  
  SET(EXTERNAL_EMBREE_DIR ${PROJECT_SOURCE_DIR}/../embree-ospray CACHE STRING    "EXTERNAL Embree source directory")
  SET(OSPRAY_EMBREE_SOURCE_DIR ${EXTERNAL_EMBREE_DIR})
  ADD_DEFINITIONS(-D__EXTERNAL_EMBREE__=1)
ELSE()
  ##################################################################
  # default branch: build ospray using local version of embree
  ##################################################################
  SET(OSPRAY_EMBREE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/ospray/embree)
ENDIF()

##################################################################
# build embree - global settings independent of target
##################################################################

ADD_DEFINITIONS(-D__EXPORT_ALL_SYMBOLS__)
ADD_DEFINITIONS(-D__FIX_RAYS__)
ADD_DEFINITIONS(-D__INTERSECTION_FILTER__)
ADD_DEFINITIONS(-D__BUFFER_STRIDE__)
ADD_DEFINITIONS(-DEMBREE_DISABLE_HAIR=1)
