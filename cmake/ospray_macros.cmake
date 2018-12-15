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

function(ospray_check_embree_feature FEATURE DESCRIPTION)
  set(FEATURE EMBREE_${FEATURE})
  if(NOT ${ARGN})
    if (${FEATURE})
      message(FATAL_ERROR "OSPRay requires Embree to be compiled "
              "without ${DESCRIPTION} (${FEATURE}=OFF).")
    endif()
  else()
    if (NOT ${FEATURE})
      message(FATAL_ERROR "OSPRay requires Embree to be compiled "
              "with support for ${DESCRIPTION} (${FEATURE}=ON).")
    endif()
  endif()
endfunction()

function(ospray_verify_embree_features)
  ospray_check_embree_feature(ISPC_SUPPORT ISPC)
  ospray_check_embree_feature(FILTER_FUNCTION "intersection filter")
  ospray_check_embree_feature(GEOMETRY_TRIANGLE "triangle geometries")
  ospray_check_embree_feature(GEOMETRY_CURVE "spline curve geometries")
  ospray_check_embree_feature(GEOMETRY_USER "user geometries")
  ospray_check_embree_feature(RAY_PACKETS "ray packets")
  ospray_check_embree_feature(BACKFACE_CULLING "backface culling" OFF)
endfunction()

macro(ospray_find_embree EMBREE_VERSION_REQUIRED)
  find_package(embree ${EMBREE_VERSION_REQUIRED} QUIET)
  if(NOT DEFINED EMBREE_INCLUDE_DIRS)
    message(FATAL_ERROR
            "We did not find Embree installed on your system. OSPRay requires"
            " an Embree installation >= v${EMBREE_VERSION_REQUIRED}, please"
            " download and extract Embree (or compile Embree from source), then"
            " set the 'embree_DIR' variable to the installation (or build)"
            " directory.")
  else()
    message(STATUS "Found Embree v${EMBREE_VERSION}: ${EMBREE_LIBRARY}")
  endif()

  set(EMBREE_LIBRARIES ${EMBREE_LIBRARY})
endmacro()

macro(ospray_determine_embree_isa_support)
  if (EMBREE_MAX_ISA STREQUAL "NONE")
    set(EMBREE_ISA_SUPPORTS_SSE2      ${EMBREE_ISA_SSE2})
    set(EMBREE_ISA_SUPPORTS_SSE4      ${EMBREE_ISA_SSE42})
    set(EMBREE_ISA_SUPPORTS_AVX       ${EMBREE_ISA_AVX})
    set(EMBREE_ISA_SUPPORTS_AVX2      ${EMBREE_ISA_AVX2})
    set(EMBREE_ISA_SUPPORTS_AVX512KNL ${EMBREE_ISA_AVX512KNL})
    set(EMBREE_ISA_SUPPORTS_AVX512SKX ${EMBREE_ISA_AVX512SKX})
  else()
    set(EMBREE_ISA_SUPPORTS_SSE2      FALSE)
    set(EMBREE_ISA_SUPPORTS_SSE4      FALSE)
    set(EMBREE_ISA_SUPPORTS_AVX       FALSE)
    set(EMBREE_ISA_SUPPORTS_AVX2      FALSE)
    set(EMBREE_ISA_SUPPORTS_AVX512KNL FALSE)
    set(EMBREE_ISA_SUPPORTS_AVX512SKX FALSE)

    if (EMBREE_MAX_ISA STREQUAL "SSE2")
      set(EMBREE_ISA_SUPPORTS_SSE2 TRUE)
    elseif (EMBREE_MAX_ISA MATCHES "SSE4\\.[12]$")
      set(EMBREE_ISA_SUPPORTS_SSE2 TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4 TRUE)
    elseif (EMBREE_MAX_ISA STREQUAL "AVX")
      set(EMBREE_ISA_SUPPORTS_SSE2 TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4 TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX  TRUE)
    elseif (EMBREE_MAX_ISA STREQUAL "AVX2")
      set(EMBREE_ISA_SUPPORTS_SSE2 TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4 TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX  TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX2 TRUE)
    elseif (EMBREE_MAX_ISA STREQUAL "AVX512KNL")
      set(EMBREE_ISA_SUPPORTS_SSE2      TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4      TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX       TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX2      TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX512KNL TRUE)
    elseif (EMBREE_MAX_ISA STREQUAL "AVX512SKX")
      set(EMBREE_ISA_SUPPORTS_SSE2      TRUE)
      set(EMBREE_ISA_SUPPORTS_SSE4      TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX       TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX2      TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX512KNL TRUE)
      set(EMBREE_ISA_SUPPORTS_AVX512SKX TRUE)
    endif()
  endif()

  if (NOT (EMBREE_ISA_SUPPORTS_SSE4
           OR EMBREE_ISA_SUPPORTS_AVX
           OR EMBREE_ISA_SUPPORTS_AVX2
           OR EMBREE_ISA_SUPPORTS_AVX512KNL
           OR EMBREE_ISA_SUPPORTS_AVX512SKX))
      message(FATAL_ERROR
              "Your Embree build needs to support at least one ISA >= SSE4.1!")
  endif()
endmacro()
