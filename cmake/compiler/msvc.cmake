## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(COMMON_CXX_FLAGS "/EHsc /MP /GR /bigobj")

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
