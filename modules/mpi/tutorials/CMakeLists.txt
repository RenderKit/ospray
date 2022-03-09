## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

macro(ospray_create_mpi_tutorial tutorial_name)
  add_executable(${tutorial_name} ${OSPRAY_RESOURCE} ${ARGN})
  target_link_libraries(${tutorial_name} PRIVATE mpi_tutorial_common)
  install(TARGETS ${tutorial_name}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT mpi
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
  ${OSPRAY_RESOURCE} ospMPIDistributedTutorial.c)

target_link_libraries(ospMPIDistributedTutorial PRIVATE
  ospray
  ${MPI_C_LIBRARIES})

target_include_directories(ospMPIDistributedTutorial PRIVATE
  $<BUILD_INTERFACE:${MPI_C_INCLUDE_PATH}>)

ospray_sign_target(ospMPIDistributedTutorial)

# C++ version
add_executable(ospMPIDistributedTutorialCpp
  ${OSPRAY_RESOURCE} ospMPIDistributedTutorial.cpp)

target_link_libraries(ospMPIDistributedTutorialCpp PRIVATE
  ospray
  rkcommon::rkcommon
  MPI::MPI_CXX
)

target_compile_definitions(ospMPIDistributedTutorialCpp PRIVATE
    -DOSPRAY_CPP_RKCOMMON_TYPES)

ospray_sign_target(ospMPIDistributedTutorialCpp)

# async version
add_executable(ospMPIDistributedTutorialAsync
  ${OSPRAY_RESOURCE} ospMPIDistributedTutorialAsync.c)

target_link_libraries(ospMPIDistributedTutorialAsync PRIVATE
  ospray
  ${MPI_C_LIBRARIES})

target_include_directories(ospMPIDistributedTutorialAsync PRIVATE
  $<BUILD_INTERFACE:${MPI_C_INCLUDE_PATH}>)

ospray_sign_target(ospMPIDistributedTutorialAsync)

# MPI Tutorial utilities library
add_library(mpi_tutorial_common STATIC
  GLFWDistribOSPRayWindow.cpp
  ${CMAKE_SOURCE_DIR}/apps/ospExamples/imgui_impl_glfw_gl3.cpp
)

target_include_directories(mpi_tutorial_common PRIVATE
  ${CMAKE_SOURCE_DIR}/apps/ospExamples/)

target_link_libraries(mpi_tutorial_common PUBLIC
  rkcommon::rkcommon
  ospray
  ospray_imgui
  glfw
  arcball_camera
  ${OPENGL_LIBRARIES}
  MPI::MPI_CXX
)


target_compile_definitions(mpi_tutorial_common PUBLIC -DOSPRAY_CPP_RKCOMMON_TYPES)

ospray_create_mpi_tutorial(ospMPIDistributedTutorialSpheres
  ospMPIDistributedTutorialSpheres.cpp
)

ospray_sign_target(ospMPIDistributedTutorialSpheres)

ospray_create_mpi_tutorial(ospMPIDistributedTutorialStructuredVolume
  ospMPIDistributedTutorialStructuredVolume.cpp
)

ospray_sign_target(ospMPIDistributedTutorialStructuredVolume)

ospray_create_mpi_tutorial(ospMPIDistributedTutorialPartiallyReplicatedData
  ospMPIDistributedTutorialPartiallyReplicatedData.cpp
)

ospray_sign_target(ospMPIDistributedTutorialPartiallyReplicatedData)

ospray_create_mpi_tutorial(ospMPIDistributedTutorialReplicatedData
  ospMPIDistributedTutorialReplicatedData.cpp
)
ospray_sign_target(ospMPIDistributedTutorialReplicatedData)

target_link_libraries(ospMPIDistributedTutorialReplicatedData PUBLIC
  ospray_testing
)

