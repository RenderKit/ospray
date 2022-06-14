## Copyright 2009 Intel Corporation
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
add_executable(ospMPIDistribTutorial
  ${OSPRAY_RESOURCE} ospMPIDistribTutorial.c)

target_link_libraries(ospMPIDistribTutorial PRIVATE
  ospray
  ${MPI_C_LIBRARIES})

target_include_directories(ospMPIDistribTutorial PRIVATE
  $<BUILD_INTERFACE:${MPI_C_INCLUDE_PATH}>)

ospray_sign_target(ospMPIDistribTutorial)

# C++ version
add_executable(ospMPIDistribTutorialCpp
  ${OSPRAY_RESOURCE} ospMPIDistribTutorial.cpp)

target_link_libraries(ospMPIDistribTutorialCpp PRIVATE
  ospray
  rkcommon::rkcommon
  MPI::MPI_CXX
)

target_compile_definitions(ospMPIDistribTutorialCpp PRIVATE
  -DOSPRAY_CPP_RKCOMMON_TYPES)

ospray_sign_target(ospMPIDistribTutorialCpp)

# async version
add_executable(ospMPIDistribTutorialAsync
  ${OSPRAY_RESOURCE} ospMPIDistribTutorialAsync.c)

target_link_libraries(ospMPIDistribTutorialAsync PRIVATE
  ospray
  ${MPI_C_LIBRARIES})

target_include_directories(ospMPIDistribTutorialAsync PRIVATE
  $<BUILD_INTERFACE:${MPI_C_INCLUDE_PATH}>)

ospray_sign_target(ospMPIDistribTutorialAsync)

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

ospray_create_mpi_tutorial(ospMPIDistribTutorialSpheres
  ospMPIDistribTutorialSpheres.cpp
)

ospray_sign_target(ospMPIDistribTutorialSpheres)

ospray_create_mpi_tutorial(ospMPIDistribTutorialVolume
  ospMPIDistribTutorialVolume.cpp
)

ospray_sign_target(ospMPIDistribTutorialVolume)

ospray_create_mpi_tutorial(ospMPIDistribTutorialPartialRepl
  ospMPIDistribTutorialPartialRepl.cpp
)

ospray_sign_target(ospMPIDistribTutorialPartialRepl)

ospray_create_mpi_tutorial(ospMPIDistribTutorialReplicated
  ospMPIDistribTutorialReplicated.cpp
)
ospray_sign_target(ospMPIDistribTutorialReplicated)

target_link_libraries(ospMPIDistribTutorialReplicated PUBLIC
  ospray_testing
)

ospray_create_mpi_tutorial(ospMPIDistributedExamples
    ospMPIDistributedExamples.cpp
)
ospray_sign_target(ospMPIDistributedExamples)

target_link_libraries(ospMPIDistributedExamples PUBLIC
  ospray_testing
)

