# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

IF (NOT THIS_IS_MIC)
  INCLUDE_DIRECTORIES(${OSPRAY_DIR}/ospray)
  
  INCLUDE_DIRECTORIES(${OSPRAY_DIR})

  ADD_LIBRARY(ospray_minisg 
    miniSG.cpp
    xml.cpp # for importRIVL
    importer.cpp
    importOBJ.cpp
    importSTL.cpp
    importMSG.cpp
    importTRI.cpp
    importRIVL.cpp
    )
  TARGET_LINK_LIBRARIES(ospray_minisg)
  # ------------------------------------------------------------
  SET_TARGET_PROPERTIES(ospray_minisg${OSPRAY_LIB_SUFFIX} 
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION  ${OSPRAY_SOVERSION})
  INSTALL(TARGETS ospray_minisg${OSPRAY_LIB_SUFFIX} DESTINATION lib)
  # ------------------------------------------------------------

ENDIF(NOT THIS_IS_MIC)