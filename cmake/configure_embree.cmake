# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_ALLOW_EXTERNAL_EMBREE OFF CACHE BOOL "Allow building ospray w/ EXTERNAL embree")
MARK_AS_ADVANCED(OSPRAY_ALLOW_EXTERNAL_EMBREE)

IF (OSPRAY_ALLOW_EXTERNAL_EMBREE)
  ##################################################################
  # if you are using this branch we assume you know what you are doing!
  ##################################################################
  MESSAGE("You have chosen to build OSPRay with a external version of embree that may or may not have been tested to work with this version of OSPRay. Use this option at your own risk.")

  # NOTE: 
  SET(EXTERNAL_EMBREE_DIR ${PROJECT_SOURCE_DIR}/../embree-ospray CACHE STRING    "EXTERNAL Embree source directory")
#  MARK_AS_ADVANCED(EMBREE_DIR)
  SET(OSPRAY_EMBREE_SOURCE_DIR ${EXTERNAL_EMBREE_DIR})
#  SET(OSPRAY_EMBREE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/ospray/external-embree)
ELSE()
  ##################################################################
  # default branch: build ospray using local version of embree
  ##################################################################
  SET(OSPRAY_EMBREE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/embree)
ENDIF()

##################################################################
# build embree - global settings independent of target
##################################################################

ADD_DEFINITIONS(-D__EXPORT_ALL_SYMBOLS__)
ADD_DEFINITIONS(-D__FIX_RAYS__)
ADD_DEFINITIONS(-D__INTERSECTION_FILTER__)
ADD_DEFINITIONS(-D__BUFFER_STRIDE__)
ADD_DEFINITIONS(-DEMBREE_DISABLE_HAIR=1)

# this section could be sooo much cleaner if embree only used
# fully-qualified include names...
SET(EMBREE_INCLUDE_DIRECTORIES
  ${OSPRAY_EMBREE_SOURCE_DIR}/ 
  ${OSPRAY_EMBREE_SOURCE_DIR}/include
  ${OSPRAY_EMBREE_SOURCE_DIR}/
  ${OSPRAY_EMBREE_SOURCE_DIR}/common
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels
  ${OSPRAY_EMBREE_SOURCE_DIR}/kernels/xeon
  )
INCLUDE_DIRECTORIES(${EMBREE_INCLUDE_DIRECTORIES})

