## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(OSPRAY_CXX_FLAGS "-fno-strict-aliasing -Wno-narrowing -Wno-unknown-pragmas")

if(OSPRAY_STRICT_BUILD)
  # Should try to fix and remove...
  set(OSPRAY_CXX_FLAGS "-Wno-aligned-new ${OSPRAY_CXX_FLAGS}")

  # Options selected for GCC 7.1+
  set(OSPRAY_CXX_FLAGS "-Wall ${OSPRAY_CXX_FLAGS}")
endif()

if(OSPRAY_WARN_AS_ERRORS)
  set(OSPRAY_CXX_FLAGS "-Werror ${OSPRAY_CXX_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${OSPRAY_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

if (APPLE)
  set(CMAKE_CXX_FLAGS "-mmacosx-version-min=10.9 ${CMAKE_CXX_FLAGS}") # we only use MacOSX 10.9 features
endif()
