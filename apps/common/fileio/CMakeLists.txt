# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

CONFIGURE_OSPRAY()

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

ADD_LIBRARY(ospray_fileio SHARED
  OSPObjectFile.cpp
  RawVolumeFile.cpp
  SymbolRegistry.cpp
  TinyXML2.cpp
)

TARGET_LINK_LIBRARIES(ospray_fileio ospray)
# ------------------------------------------------------------
SET_TARGET_PROPERTIES(ospray_fileio${OSPRAY_LIB_SUFFIX} 
  PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
INSTALL(TARGETS ospray_fileio${OSPRAY_LIB_SUFFIX} DESTINATION lib)

