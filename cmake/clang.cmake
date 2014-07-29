# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_ARCH_FLAGS__SSSE3 "-msse3")
SET(OSPRAY_ARCH_FLAGS__SSSE3 "-mssse3")
SET(OSPRAY_ARCH_FLAGS__SSE41 "-msse4.1")
SET(OSPRAY_ARCH_FLAGS__SSE42 "-msse4.2")
SET(OSPRAY_ARCH_FLAGS__AVX   "-mavx")
SET(OSPRAY_ARCH_FLAGS__AVX2  "-mavx2 -mfma -mlzcnt -mbmi -mbmi2")

SET(CMAKE_CXX_COMPILER "clang++")
SET(CMAKE_C_COMPILER "clang")
SET(CMAKE_CXX_FLAGS "-fPIC")
SET(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g -O2 -ftree-ter")
SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -Wstrict-aliasing=0 -ffast-math ")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O3 -Wstrict-aliasing=0 -ffast-math ")
SET(CMAKE_EXE_LINKER_FLAGS "")

IF (NOT RTCORE_EXPORT_ALL_SYMBOLS)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden -fvisibility=hidden")
ENDIF()

IF (APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.7")
ENDIF (APPLE)
