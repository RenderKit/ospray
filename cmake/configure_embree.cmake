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

SET(EMBREE_VERSION_REQUIRED 2.13.0)

# Find existing Embree on the machine #######################################

FIND_PACKAGE(embree ${EMBREE_VERSION_REQUIRED} QUIET)
IF(NOT DEFINED EMBREE_INCLUDE_DIRS)
  MESSAGE(FATAL_ERROR
          "We did not find Embree installed on your system. OSPRay requires an"
          " Embree installation >= v${EMBREE_VERSION_REQUIRED}, please download"
          " and extract Embree or compile Embree from source, then set the"
          " 'embree_DIR' variable to where it is installed.")
ENDIF()

# Check for required Embree features  #######################################

OSPRAY_CHECK_EMBREE_FEATURE(ISPC_SUPPORT ISPC)
OSPRAY_CHECK_EMBREE_FEATURE(INTERSECTION_FILTER "intersection filter")
OSPRAY_CHECK_EMBREE_FEATURE(INTERSECTION_FILTER_RESTORE "intersection filter")
OSPRAY_CHECK_EMBREE_FEATURE(GEOMETRY_TRIANGLES "triangle geometries")
OSPRAY_CHECK_EMBREE_FEATURE(GEOMETRY_USER "user geometries")
OSPRAY_CHECK_EMBREE_FEATURE(RAY_PACKETS "ray packets")
OSPRAY_CHECK_EMBREE_FEATURE(BACKFACE_CULLING "backface culling" OFF)

# also remove Embree's TBB libs if OSPRay uses TBB to avoid problems with
# cyclic rpath
IF(NOT DEFINED EMBREE_LIBRARIES OR OSPRAY_TASKING_TBB)
  SET(EMBREE_LIBRARIES ${EMBREE_LIBRARY})
ELSE()
  # check if we need to add TBB to EMBREE_LIBRARIES
  IF((EMBREE_TASKING_TBB OR (${EMBREE_TASKING_SYSTEM} STREQUAL "TBB"))
      AND NOT EMBREE_USE_PACKAGED_TBB)
    OSPRAY_WARN_ONCE(EMBREE_FORCE_TBB
      "You *MUST* have TBB installed based on the Embree we found!")
    FIND_PACKAGE(TBB REQUIRED)
    SET(EMBREE_LIBRARIES ${EMBREE_LIBRARIES} ${TBB_LIBRARIES})
  ENDIF()
ENDIF()

IF (EMBREE_MAX_ISA STREQUAL "NONE")
  SET(EMBREE_ISA_SUPPORTS_SSE4   ${EMBREE_ISA_SSE42})
  SET(EMBREE_ISA_SUPPORTS_AVX    ${EMBREE_ISA_AVX})
  SET(EMBREE_ISA_SUPPORTS_AVX2   ${EMBREE_ISA_AVX2})
  SET(EMBREE_ISA_SUPPORTS_AVX512 ${EMBREE_ISA_AVX512KNL})
ELSE()
  SET(EMBREE_ISA_SUPPORTS_SSE4   FALSE)
  SET(EMBREE_ISA_SUPPORTS_AVX    FALSE)
  SET(EMBREE_ISA_SUPPORTS_AVX2   FALSE)
  SET(EMBREE_ISA_SUPPORTS_AVX512 FALSE)


  IF (EMBREE_MAX_ISA MATCHES "SSE4\\.[12]$")
    SET(EMBREE_ISA_SUPPORTS_SSE4 TRUE)
  ELSEIF (EMBREE_MAX_ISA STREQUAL "AVX")
    SET(EMBREE_ISA_SUPPORTS_SSE4 TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX  TRUE)
  ELSEIF (EMBREE_MAX_ISA STREQUAL "AVX2")
    SET(EMBREE_ISA_SUPPORTS_SSE4 TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX  TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX2 TRUE)
  ELSEIF (EMBREE_MAX_ISA STREQUAL "AVX512KNL")
    SET(EMBREE_ISA_SUPPORTS_SSE4   TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX    TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX2   TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX512 TRUE)
  ENDIF()
ENDIF()

IF(NOT EMBREE_ISA_SUPPORTS_SSE4)
    MESSAGE(FATAL_ERROR
            "Your Embree build needs to support at least SSE4.1!")
ENDIF()

# Configure OSPRay ISA last after we've detected what we got w/ Embree
OSPRAY_CONFIGURE_ISPC_ISA()
