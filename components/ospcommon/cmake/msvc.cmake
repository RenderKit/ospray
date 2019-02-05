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

set(COMMON_CXX_FLAGS "/EHsc /MP /GR /bigobj /D NOMINMAX ")

set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE}        ${COMMON_CXX_FLAGS} /Ox /fp:fast /Oi /Gy ")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${COMMON_CXX_FLAGS} /Ox /fp:fast /Oi /Gy ")

# optionally use static runtime library
option(USE_STATIC_RUNTIME "Use the static version of the C/C++ runtime library.")
mark_as_advanced(USE_STATIC_RUNTIME)
if (USE_STATIC_RUNTIME)
  foreach(FLAG
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_C_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO
  )
    string(REPLACE "/MD" "/MT" ${FLAG} ${${FLAG}})
  endforeach()
else()
  # remove libmmd dependency
  if (OSPRAY_COMPILER_ICC)
    # prevents sin(x),cos(x) -> sincos(x) optimization, which is only present in libmmd
    foreach(FLAG
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
    )
      string(APPEND ${FLAG} " /Qfast-transcendentals-")
    endforeach()
    # use default math lib instead of libmmd[d]
    string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /nodefaultlib:libmmdd.lib")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO " /nodefaultlib:libmmd.lib")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " /nodefaultlib:libmmd.lib")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /nodefaultlib:libmmdd.lib")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO " /nodefaultlib:libmmd.lib")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " /nodefaultlib:libmmd.lib")
  endif()
endif()
