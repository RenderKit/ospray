# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_MODULE_SEISMIC OFF CACHE BOOL "Build seismic volume file support module")

IF (NOT THIS_IS_MIC)
  IF (OSPRAY_MODULE_SEISMIC)

    CONFIGURE_OSPRAY()

    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
    INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)

    LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/modules/seismic)
    FIND_PACKAGE(FreeDDS REQUIRED) 
    INCLUDE_DIRECTORIES(${FreeDDS_INCLUDE_DIRS})

    ADD_LIBRARY(ospray_module_seismic${OSPRAY_LIB_SUFFIX} SHARED
      SeismicVolumeFile.cpp
      SymbolRegistry.cpp
      ../../apps/common/fileio/TinyXML2.cpp
    )

    TARGET_LINK_LIBRARIES(ospray_module_seismic${OSPRAY_LIB_SUFFIX} ${FreeDDS_LIBRARIES})

    SET_TARGET_PROPERTIES(ospray_module_seismic${OSPRAY_LIB_SUFFIX}
      PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
    INSTALL(TARGETS ospray_module_seismic${OSPRAY_LIB_SUFFIX} DESTINATION lib)

  ENDIF (OSPRAY_MODULE_SEISMIC)
ENDIF (NOT THIS_IS_MIC)
