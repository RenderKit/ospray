# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################

SET(OSPRAY_ARCH_SSE3  "-xsse3")
SET(OSPRAY_ARCH_SSSE3 "-xssse3")
SET(OSPRAY_ARCH_SSE41 "-xsse4.1")
SET(OSPRAY_ARCH_SSE42 "-xsse4.2")
SET(OSPRAY_ARCH_SSE   "-xsse4.2")
SET(OSPRAY_ARCH_AVX   "-xAVX")
SET(OSPRAY_ARCH_AVX2  "-xCORE-AVX2")

SET(CMAKE_CXX_COMPILER "icpc")
SET(CMAKE_C_COMPILER "icc")
#SET(CMAKE_CXX_FLAGS "-Wall -fPIC -static-intel")
SET(CMAKE_CXX_FLAGS "-Wall -fPIC -static-intel -openmp")
SET(CMAKE_CXX_FLAGS_DEBUG          "-DDEBUG  -g")
SET(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG    -O3 -no-ansi-alias -restrict -fp-model fast -fimf-precision=low -no-prec-div -no-prec-sqrt -fma -no-inline-max-total-size -inline-factor=200 ")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O3 -no-ansi-alias -restrict -fp-model fast -fimf-precision=low -no-prec-div -no-prec-sqrt  -fma  -no-inline-max-total-size -inline-factor=200")
SET(CMAKE_EXE_LINKER_FLAGS "") 

IF (APPLE)
  SET (CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS_INIT} -dynamiclib)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.7")
ENDIF (APPLE)
