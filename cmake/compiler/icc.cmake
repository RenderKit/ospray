## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(OSPRAY_CXX_FLAGS "-fno-strict-aliasing -no-ansi-alias -Wno-unknown-pragmas")

if (OSPRAY_STRICT_BUILD)
  set(OSPRAY_CXX_FLAGS "-Wall ${OSPRAY_CXX_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${OSPRAY_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

if (APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "-dynamiclib ${CMAKE_SHARED_LINKER_FLAGS_INIT}")
  set(CMAKE_CXX_FLAGS "-mmacosx-version-min=10.8 ${CMAKE_CXX_FLAGS}") # we only use MacOSX 10.8 features
  set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}") # link against C++11 stdlib
endif()

# enable -static-intel and avoid to export ICC specific symbols from OSPRay
set(CMAKE_CXX_FLAGS "-static-intel ${CMAKE_CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--exclude-libs=ALL ${CMAKE_SHARED_LINKER_FLAGS}")
