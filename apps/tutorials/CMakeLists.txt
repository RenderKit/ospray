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

macro(ospray_create_tutorial tutorial_name)
  add_executable(${tutorial_name} ${ARGN})
  target_link_libraries(${tutorial_name} PRIVATE tutorial_common)
  if (WIN32)
    set_target_properties(${tutorial_name} PROPERTIES VERSION ${OSPRAY_VERSION})
  endif()
  install(TARGETS ${tutorial_name}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT apps
  )
endmacro()

# prefer libGL over libOpenGl for better compatibility with SWR
set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)
find_package(glfw3 REQUIRED)

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

add_library(tutorial_common STATIC
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  tutorial_util.cpp
  imgui/imgui_impl_glfw_gl3.cpp
)

ospray_install_library(tutorial_common COMPONENT apps)

target_link_libraries(tutorial_common PUBLIC
  ospcommon::ospcommon
  ospray
  ospray_testing
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

ospray_create_tutorial(ospTutorialBoxes ospTutorialBoxes.cpp)
ospray_create_tutorial(ospTutorialSpheres ospTutorialSpheres.cpp)
ospray_create_tutorial(ospTutorialBouncingSpheres ospTutorialBouncingSpheres.cpp)
ospray_create_tutorial(ospTutorialSubdivision ospTutorialSubdivision.cpp)
ospray_create_tutorial(ospTutorialStructuredVolume ospTutorialStructuredVolume.cpp)
ospray_create_tutorial(ospTutorialUnstructuredVolume ospTutorialUnstructuredVolume.cpp)
ospray_create_tutorial(ospTutorialAMRVolume ospTutorialAMRVolume.cpp)
ospray_create_tutorial(ospTutorialImageOperations ospTutorialImageOperations.cpp)
ospray_create_tutorial(ospTutorialStreamlines ospTutorialStreamlines.cpp)
ospray_create_tutorial(ospTutorialCurves ospTutorialCurves.cpp)
ospray_create_tutorial(ospTutorialQuadMesh ospTutorialQuadMesh.cpp)
ospray_create_tutorial(ospTutorialCylinders ospTutorialCylinders.cpp)
ospray_create_tutorial(ospTutorialVolumePathTracer ospTutorialVolumePathTracer.cpp)
