MESSAGE("including ospcommon... bin dir ${LIBRARY_OUTPUT_PATH}")

IF (DEFINED FORCED_OSPCOMMON_LIB_NAME) 
  SET(OSPCOMMON_LIB_NAME ${FORCED_OSPCOMMON_LIB_NAME})
ELSE()
  SET(OSPCOMMON_LIB_NAME ospray_ospcommon)
ENDIF()

ADD_LIBRARY(${OSPCOMMON_LIB_NAME}
  SHARED
# STATIC
   common.cpp
   FileName.cpp
   sysinfo.cpp
   malloc.cpp
   library.cpp
   thread.cpp
   )

		     

		      
