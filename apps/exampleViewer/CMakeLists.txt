## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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

find_package(OpenGL REQUIRED)

if (0)
##############################################################
# Find or build GLFW
##############################################################

find_package(GLFW)

if (NOT GLFW_FOUND)
  message(STATUS "Building GLFW from included source")

  set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

  if (USE_STATIC_RUNTIME)
    set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF)
  else()
    set(USE_MSVC_RUNTIME_LIBRARY_DLL ON)
  endif()
  set(USE_MSVC_RUNTIME_LIBRARY_DLL ${USE_MSVC_RUNTIME_LIBRARY_DLL} CACHE BOOL "" FORCE)
  MARK_AS_ADVANCED(USE_MSVC_RUNTIME_LIBRARY_DLL)

  add_subdirectory(common/glfw)
  set(GLFW_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/common/glfw/include)
  set(GLFW_LIBRARY glfw)
  set(GLFW_LIBRARIES ${GLFW_LIBRARY})
else()
  message(STATUS "Using GLFW found in the environment")
endif()
endif()


ADD_DEFINITIONS(-DDW=1)

##############################################################
# Build support libs
##############################################################

add_subdirectory(common)
add_subdirectory(widgets)

##############################################################
# Build app
##############################################################

include_directories(
  ${CMAKE_SOURCE_DIR}
  common/gl3w
  common/imgui
  ${OPENGL_INCLUDE_DIR}
)

ospray_create_application(ospExampleViewerSg
  ospExampleViewerSg.cpp
  LINK
  ${OPENGL_LIBRARIES}
  ${GLFW_LIBRARY}
  gl3w
  imgui
  ospray
  ospray_common
  ospray_commandline
  ospray_imgui3d_sg
  ospray_sg
)

ospray_create_application(ospExampleViewer
  ospExampleViewer.cpp
  LINK
  ${OPENGL_LIBRARIES}
  ${GLFW_LIBRARY}
  gl3w
  imgui
  ospray
  ospray_common
  ospray_commandline
  ospray_imgui3d
)
