# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

IF (NOT THIS_IS_MIC)
  INCLUDE_DIRECTORIES(${OSPRAY_DIR}/ospray)
  
  FIND_PACKAGE(LibXml2)
  
  IF (NOT LIBXML2_FOUND)
    MESSAGE("could not find libxml!")
  ENDIF()

  INCLUDE_DIRECTORIES(${OSPRAY_DIR})
  INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
  ADD_DEFINITIONS(${LIBXML2_DEFINITIONS})

  ADD_LIBRARY(ospray_minisg 
    miniSG.cpp
    importer.cpp
    importOBJ.cpp
    importSTL.cpp
    importMSG.cpp
    importTRI.cpp
    importRIVL.cpp
    )
  TARGET_LINK_LIBRARIES(ospray_minisg ${LIBXML2_LIBRARIES})
  # ------------------------------------------------------------
  SET_TARGET_PROPERTIES(ospray_minisg${OSPRAY_LIB_SUFFIX} 
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION  ${OSPRAY_SOVERSION})
  INSTALL(TARGETS ospray_minisg${OSPRAY_LIB_SUFFIX} DESTINATION lib)
  # ------------------------------------------------------------

ENDIF(NOT THIS_IS_MIC)