IF (NOT THIS_IS_MIC)
  INCLUDE_DIRECTORIES(${OSPRAY_DIR}/ospray)
  
  INCLUDE_DIRECTORIES(${OSPRAY_DIR})

  ADD_LIBRARY(ospray_minisg 
    miniSG.cpp
    importer.cpp
    importOBJ.cpp
    importSTL.cpp
    importMSG.cpp
    importTRI.cpp
    importX3D.cpp
    importRIVL.cpp
    )
  TARGET_LINK_LIBRARIES(ospray_minisg ospray_xml)
  # ------------------------------------------------------------
  SET_TARGET_PROPERTIES(ospray_minisg${OSPRAY_LIB_SUFFIX} 
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION  ${OSPRAY_SOVERSION})
  INSTALL(TARGETS ospray_minisg${OSPRAY_LIB_SUFFIX} DESTINATION lib)
  # ------------------------------------------------------------

ENDIF(NOT THIS_IS_MIC)