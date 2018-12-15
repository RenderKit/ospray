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

##############################################################
# Find or build GLFW
##############################################################

find_package(glfw3 QUIET)

if (NOT glfw3_FOUND)
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
  mark_as_advanced(USE_MSVC_RUNTIME_LIBRARY_DLL)

  add_subdirectory(common/glfw)
  message(STATUS "GLFW not found! Building GLFW in the OSPRay source tree")
else()
  message(STATUS "Using GLFW found in the environment")
endif()

#NOTE(jda) - on Ubuntu16.04, the 'glfw' target doesn't exist here "yet", but
#            still works as a target for linking the viewer app (???)
if (TARGET glfw)
  get_property(glfw3_INCLUDE_DIRS TARGET glfw PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
endif()

set(GLFW_INCLUDE_DIRS ${glfw3_INCLUDE_DIRS}
    CACHE INTERNAL "Found GLFW includes")

if (USE_STATIC_RUNTIME)
  string(REPLACE "/MDd" "/MTd" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
endif()
