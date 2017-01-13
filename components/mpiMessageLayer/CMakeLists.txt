PROJECT(MAML)
CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

SET(MAML_DIR  "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "" FORCE)

IF (INCLUDED_AS_AN_OSPRAY_COMPONENT)

  OSPRAY_BUILD_COMPONENT(mpiCommon)
  OSPRAY_CONFIGURE_MPI()
  
  # this was included as a submodule from something that already
  # provides the ospcommon library (likely, ospray), so don't try to
  # build it again. Also, this means that all external dependencies
  # such as tbb, include dirs etc are already set
ELSE()
  FIND_PACKAGE(MPI REQUIRED)

  INCLUDE_DIRECTORIES(${MAML_DIR})
  INCLUDE_DIRECTORIES(${MAML_DIR}/include)
  
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  INCLUDE(ospcommon/FindTBB.cmake)
  
  INCLUDE_DIRECTORIES(${TBB_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(SYSTEM ${MPI_INCLUDE_PATH})

  # ------------------------------------------------------------------
  # OSPCommon library
  # ------------------------------------------------------------------
  ADD_LIBRARY(ospcommon SHARED
    ospcommon/common.cpp
    ospcommon/library.cpp
    ospcommon/FileName.cpp
    ospcommon/sysinfo.cpp
    )
  TARGET_LINK_LIBRARIES(ospray_common
    ${TBB_LIBRARY}
    )
ENDIF()

SET(MAML_LIBRARY maml)
IF (";${MAML_LIBRARY_NAME_PREFIX}" STREQUAL ";")
  SET(MAML_LIBRARY maml)
ELSE()
  SET(MAML_LIBRARY "${MAML_LIBRARY_NAME_PREFIX}maml")
ENDIF()
# ------------------------------------------------------------------
# The MAML library itself
# ------------------------------------------------------------------
ADD_LIBRARY(${MAML_LIBRARY} SHARED
  maml/maml.cpp
  maml/Context.cpp
  )
TARGET_LINK_LIBRARIES(${MAML_LIBRARY}
  ospray_mpi_common
  )

IF (NOT INCLUDED_AS_AN_OSPRAY_COMPONENT)
  # ------------------------------------------------------------------
  # tests and examples
  # ------------------------------------------------------------------
  ADD_EXECUTABLE(mamlTest
    apps/mamlTest.cpp
    )
  TARGET_LINK_LIBRARIES(mamlTest
    ${MAML_LIBRARY}
    )

  ADD_EXECUTABLE(mamlTestMessageSwarm
    apps/testMessageSwarm.cpp
    )
  TARGET_LINK_LIBRARIES(mamlTestMessageSwarm
    ${MAML_LIBRARY}
    )
ENDIF()

