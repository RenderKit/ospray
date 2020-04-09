## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

macro(ospray_create_mpi_tutorial tutorial_name)
  add_executable(${tutorial_name} ${OSPRAY_MPI_RESOURCE} ${ARGN})
  target_link_libraries(${tutorial_name} PRIVATE mpi_tutorial_common)
  install(TARGETS ${tutorial_name}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT apps
  )
endmacro()

find_package(MPI REQUIRED)

# prefer libGL over libOpenGl for better compatibility with SWR
set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)

find_package(glfw3 REQUIRED)
if (TARGET glfw)
  get_property(glfw3_INCLUDE_DIRS TARGET glfw
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  )
endif()

# build ospTutorial, for demonstration and API testing
add_executable(ospMPIDistributedTutorial
  ${OSPRAY_MPI_RESOURCE} ospMPIDistributedTutorial.c)

target_link_libraries(ospMPIDistributedTutorial PRIVATE
  ospray::ospray
  ${MPI_C_LIBRARIES})

target_include_directories(ospMPIDistributedTutorial PRIVATE
  $<BUILD_INTERFACE:${MPI_C_INCLUDE_PATH}>)

# C++ version
add_executable(ospMPIDistributedTutorialCpp
  ${OSPRAY_MPI_RESOURCE} ospMPIDistributedTutorial.cpp)

target_link_libraries(ospMPIDistributedTutorialCpp PRIVATE
  ospray::ospray
  ospcommon::ospcommon
  ${MPI_CXX_LIBRARIES})

target_include_directories(ospMPIDistributedTutorialCpp PRIVATE
  $<BUILD_INTERFACE:${MPI_CXX_INCLUDE_PATH}>)

# async version
add_executable(ospMPIDistributedTutorialAsync
  ${OSPRAY_MPI_RESOURCE} ospMPIDistributedTutorialAsync.c)

target_link_libraries(ospMPIDistributedTutorialAsync PRIVATE
  ospray::ospray
  ${MPI_C_LIBRARIES})

target_include_directories(ospMPIDistributedTutorialAsync PRIVATE
  $<BUILD_INTERFACE:${MPI_C_INCLUDE_PATH}>)

# MPI Tutorial utilities library
add_library(mpi_tutorial_common STATIC
  GLFWDistribOSPRayWindow.cpp
  imgui_impl_glfw_gl3.cpp
)

target_link_libraries(mpi_tutorial_common PUBLIC
  ospcommon::ospcommon
  ospray::ospray
  imgui
  glfw
  arcball_camera
  ${OPENGL_LIBRARIES}
  ${MPI_CXX_LIBRARIES}
)

target_include_directories(mpi_tutorial_common PUBLIC
  $<BUILD_INTERFACE:${MPI_CXX_INCLUDE_PATH}>
)

ospray_create_mpi_tutorial(ospMPIDistributedTutorialSpheres
  ospMPIDistributedTutorialSpheres.cpp
)

ospray_create_mpi_tutorial(ospMPIDistributedTutorialStructuredVolume
  ospMPIDistributedTutorialStructuredVolume.cpp
)

ospray_create_mpi_tutorial(ospMPIDistributedTutorialPartiallyReplicatedData
  ospMPIDistributedTutorialPartiallyReplicatedData.cpp
)

ospray_create_mpi_tutorial(ospMPIDistributedTutorialReplicatedData
  ospMPIDistributedTutorialReplicatedData.cpp
)

target_link_libraries(ospMPIDistributedTutorialReplicatedData PUBLIC
  ospray::ospray_testing
)

