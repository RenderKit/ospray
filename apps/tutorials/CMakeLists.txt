## ======================================================================== ##
## Copyright 2018-2019 Intel Corporation                                    ##
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
add_executable(ospTutorial ospTutorial.c)
target_link_libraries(ospTutorial PRIVATE ospray)
# C++ version
add_executable(ospTutorialCpp ospTutorial.cpp)
target_link_libraries(ospTutorialCpp PRIVATE ospray_sdk)
# async version
add_executable(ospTutorialAsync ospTutorialAsync.c)
target_link_libraries(ospTutorialAsync PRIVATE ospray)

## Create a static library of all common tutorial bits ##

ospray_create_library(tutorial_common STATIC
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  tutorial_util.cpp
  imgui/imgui_impl_glfw_gl3.cpp
)

target_link_libraries(tutorial_common PUBLIC
  ospray
  ospray_testing
  ospray_common
  ospray_imgui
  glfw
  ${OPENGL_LIBRARIES}
)

target_include_directories(tutorial_common INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/tutorials>
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  COMPONENT devel
  FILES_MATCHING
  PATTERN *.h
  PATTERN imgui EXCLUDE
)

## Build the actual tutorials ##

ospray_create_application(ospTutorialBoxes ospTutorialBoxes.cpp)
target_link_libraries(ospTutorialBoxes PRIVATE tutorial_common)

ospray_create_application(ospTutorialSpheres ospTutorialSpheres.cpp)
target_link_libraries(ospTutorialSpheres PRIVATE tutorial_common)

ospray_create_application(ospTutorialBouncingSpheres
  ospTutorialBouncingSpheres.cpp
)
target_link_libraries(ospTutorialBouncingSpheres PRIVATE tutorial_common)

ospray_create_application(ospTutorialSubdivision ospTutorialSubdivision.cpp)
target_link_libraries(ospTutorialSubdivision PRIVATE tutorial_common)

ospray_create_application(ospTutorialStructuredVolume
  ospTutorialStructuredVolume.cpp
)
target_link_libraries(ospTutorialStructuredVolume PRIVATE tutorial_common)

ospray_create_application(ospTutorialUnstructuredVolume
  ospTutorialUnstructuredVolume.cpp
)
target_link_libraries(ospTutorialUnstructuredVolume PRIVATE tutorial_common)

ospray_create_application(ospTutorialPixelOps ospTutorialPixelOps.cpp)
target_link_libraries(ospTutorialPixelOps PRIVATE tutorial_common)
