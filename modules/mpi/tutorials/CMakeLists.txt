## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

set(OSPRAY_DEFAULT_COMPONENT apps)

find_package(MPI REQUIRED)

# prefer libGL over libOpenGl for better compatibility with SWR
set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)

find_package(glfw3 QUIET)
if (TARGET glfw)
  get_property(glfw3_INCLUDE_DIRS TARGET glfw
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  )
endif()

# build ospTutorial, for demonstration and API testing
add_executable(ospMPIDistributedTutorial ospMPIDistributedTutorial.c)
target_link_libraries(ospMPIDistributedTutorial PRIVATE
  ospray ${MPI_C_LIBRARIES})
target_include_directories(ospMPIDistributedTutorial PRIVATE
  $<BUILD_INTERFACE:${MPI_C_INCLUDE_PATH}>)

# C++ version
add_executable(ospMPIDistributedTutorialCpp ospMPIDistributedTutorial.cpp)
target_link_libraries(ospMPIDistributedTutorialCpp PRIVATE
  ospray ospcommon::ospcommon ${MPI_CXX_LIBRARIES})
target_include_directories(ospMPIDistributedTutorialCpp PRIVATE
  $<BUILD_INTERFACE:${MPI_CXX_INCLUDE_PATH}>)

# async version
add_executable(ospMPIDistributedTutorialAsync ospMPIDistributedTutorialAsync.c)
target_link_libraries(ospMPIDistributedTutorialAsync PRIVATE
  ospray ${MPI_C_LIBRARIES})
target_include_directories(ospMPIDistributedTutorialAsync PRIVATE
  $<BUILD_INTERFACE:${MPI_C_INCLUDE_PATH}>)

set(OSPRAY_TUTORIALS_DIR "${OSPRay_SOURCE_DIR}/apps/tutorials")

ospray_create_application(ospMPIDistributedTutorialSpheres
  ospMPIDistributedTutorialSpheres.cpp
  GLFWDistribOSPRayWindow.cpp
  ${OSPRAY_TUTORIALS_DIR}/ArcballCamera.cpp
  ${OSPRAY_TUTORIALS_DIR}/imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospMPIDistributedTutorialSpheres
PRIVATE
  ospray
  ospray_testing
  ospcommon::ospcommon
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
  ${MPI_CXX_LIBRARIES}
)

target_include_directories(ospMPIDistributedTutorialSpheres PRIVATE
  $<BUILD_INTERFACE:${OSPRAY_TUTORIALS_DIR}>
  $<BUILD_INTERFACE:${MPI_CXX_INCLUDE_PATH}>)

ospray_create_application(ospMPIDistributedTutorialStructuredVolume
  ospMPIDistributedTutorialStructuredVolume.cpp
  GLFWDistribOSPRayWindow.cpp
  ${OSPRAY_TUTORIALS_DIR}/ArcballCamera.cpp
  ${OSPRAY_TUTORIALS_DIR}/imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospMPIDistributedTutorialStructuredVolume
PRIVATE
  ospray
  ospray_testing
  ospcommon::ospcommon
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
  ${MPI_CXX_LIBRARIES}
)

target_include_directories(ospMPIDistributedTutorialStructuredVolume PRIVATE
  $<BUILD_INTERFACE:${OSPRAY_TUTORIALS_DIR}>
  $<BUILD_INTERFACE:${MPI_CXX_INCLUDE_PATH}>)

ospray_create_application(ospMPIDistributedTutorialPartiallyReplicatedData
  ospMPIDistributedTutorialPartiallyReplicatedData.cpp
  GLFWDistribOSPRayWindow.cpp
  ${OSPRAY_TUTORIALS_DIR}/ArcballCamera.cpp
  ${OSPRAY_TUTORIALS_DIR}/imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospMPIDistributedTutorialPartiallyReplicatedData
PRIVATE
  ospray
  ospray_testing
  ospcommon::ospcommon
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
  ${MPI_CXX_LIBRARIES}
)

target_include_directories(ospMPIDistributedTutorialPartiallyReplicatedData
  PRIVATE
  $<BUILD_INTERFACE:${OSPRAY_TUTORIALS_DIR}>
  $<BUILD_INTERFACE:${MPI_CXX_INCLUDE_PATH}>)


ospray_create_application(ospMPIDistributedTutorialReplicatedData
  ospMPIDistributedTutorialReplicatedData.cpp
  GLFWDistribOSPRayWindow.cpp
  ${OSPRAY_TUTORIALS_DIR}/ArcballCamera.cpp
  ${OSPRAY_TUTORIALS_DIR}/imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospMPIDistributedTutorialReplicatedData
PRIVATE
  ospray
  ospray_testing
  ospcommon::ospcommon
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
  ${MPI_CXX_LIBRARIES}
)

target_include_directories(ospMPIDistributedTutorialReplicatedData
  PRIVATE
  $<BUILD_INTERFACE:${OSPRAY_TUTORIALS_DIR}>
  $<BUILD_INTERFACE:${MPI_CXX_INCLUDE_PATH}>)
