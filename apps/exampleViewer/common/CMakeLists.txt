## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
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

if (NOT WIN32)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

##############################################################
# Find or build GLFW
##############################################################

find_package(GLFW)

if (NOT GLFW_FOUND)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL    OFF CACHE BOOL "" FORCE)

  mark_as_advanced(GLFW_BUILD_DOCS)
  mark_as_advanced(GLFW_DOCUMENT_INTERNALS)
  mark_as_advanced(GLFW_INSTALL)
  mark_as_advanced(GLFW_USE_MIR)
  mark_as_advanced(GLFW_USE_WAYLAND)

  if (USE_STATIC_RUNTIME)
    set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF)
  else()
    set(USE_MSVC_RUNTIME_LIBRARY_DLL ON)
  endif()
  set(USE_MSVC_RUNTIME_LIBRARY_DLL ${USE_MSVC_RUNTIME_LIBRARY_DLL} CACHE BOOL "" FORCE)
  MARK_AS_ADVANCED(USE_MSVC_RUNTIME_LIBRARY_DLL)

  add_subdirectory(glfw)
  set(GLFW_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/common/glfw/include
      CACHE INTERNAL "Included GLFW includes")
  set(GLFW_LIBRARY glfw CACHE INTERNAL "Included GLFW libs")
  set(GLFW_LIBRARIES ${GLFW_LIBRARY} CACHE INTERNAL "Included GLFW libs")
  message(STATUS "GLFW not found! Building GLFW in the OSPRay source tree")
else()
  set(GLFW_INCLUDE_DIRS ${GLFW_INCLUDE_DIRS}
      CACHE INTERNAL "Found GLFW includes")
  set(GLFW_LIBRARY ${GLFW_LIBRARY} CACHE INTERNAL "Found GLFW libs")
  set(GLFW_LIBRARIES ${GLFW_LIBRARY} CACHE INTERNAL "Foudn GLFW libs")
  message(STATUS "Using GLFW found in the environment")
endif()

IF (USE_STATIC_RUNTIME)
  STRING(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  STRING(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
  STRING(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
ENDIF()

##############################################################
# Build imgui
##############################################################

add_subdirectory(imgui)

##############################################################
# Build ospImGui utilities
##############################################################

add_subdirectory(util)
