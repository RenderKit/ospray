  
OSPRAY_BUILD_COMPONENT(mpiCommon)

SET(MAML_LIBRARY ospray_mpi_maml)
SET(MAML_DIR ${CMAKE_CURRENT_SOURCE_DIR})

OSPRAY_CONFIGURE_MPI()

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

