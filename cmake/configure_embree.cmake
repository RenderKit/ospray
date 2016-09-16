## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
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

# -------------------------------------------------------
# Find or build Embree
# -------------------------------------------------------

SET(EMBREE_VERSION_REQUIRED 2.7.1)

# Do a check to see if we can find the system Embree
IF(NOT DEFINED LAST_CONFIG_USED_EXTERNAL_EMBREE)
  FIND_PACKAGE(embree ${EMBREE_VERSION_REQUIRED} QUIET)
  IF(NOT DEFINED EMBREE_INCLUDE_DIRS)
    MESSAGE(WARNING
            "We did not find Embree installed on your system. If you would"
            " like to use a newer version of Embree than the one in the"
            " OSPRay source tree (v2.7.1), please download and extract or"
            " compile Embree from source, set the 'embree_DIR' environment"
            " variable to where it is installed, and re-enable the"
            " OSPRAY_USE_EXTERNAL_EMBREE CMake option.")
    SET(OSPRAY_USE_EXTERNAL_EMBREE OFF CACHE BOOL "" FORCE)
  ENDIF()
ENDIF()

IF(OSPRAY_USE_EXTERNAL_EMBREE)

  # Clear out embree directories if they were previously populated by an
  # internal build
  IF(NOT ${LAST_CONFIG_USED_EXTERNAL_EMBREE})
    UNSET(EMBREE_INCLUDE_DIRS)
    UNSET(EMBREE_LIBRARIES)
    UNSET(EMBREE_LIBRARY)
    UNSET(EMBREE_LIBRARY_XEONPHI)
  ENDIF()

  # Find existing Embree on the machine #######################################

  FIND_PACKAGE(embree ${EMBREE_VERSION_REQUIRED} REQUIRED)
  SET(LAST_CONFIG_USED_EXTERNAL_EMBREE ON CACHE INTERNAL "" FORCE)

  IF(OSPRAY_MIC AND NOT EMBREE_VERSION VERSION_LESS "2.10.0")
    MESSAGE(FATAL_ERROR "The found Embree version ${EMBREE_VERSION} does not"
            " support KNC anymore. Either disable the OSPRAY_BUILD_MIC_SUPPORT"
            " option or use an Embree v2.9 or earlier.")
  ENDIF()

  # NOTE(jda) - EMBREE_LIBRARIES is not defined until at lest v2.10.0, for now
  #             create a "faked" EMBREE_LIBRARIES until we set our min version
  #             to >= 2.10.0 of Embree
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

  SET(EMBREE_ISA_SUPPORTS_AVX    FALSE)
  SET(EMBREE_ISA_SUPPORTS_AVX2   FALSE)
  SET(EMBREE_ISA_SUPPORTS_AVX512 FALSE)

  # Workaround - name changed from EMBREE_ISA to EMBREE_MAX_ISA in 2.10 -> 2.11
  IF (DEFINED EMBREE_ISA)
    SET(EMBREE_ISA_NAME EMBREE_ISA)
  ELSE ()
    SET(EMBREE_ISA_NAME EMBREE_MAX_ISA)
  ENDIF()

  IF (${${EMBREE_ISA_NAME}} STREQUAL "AVX")
    SET(EMBREE_ISA_SUPPORTS_AVX TRUE)
  ELSEIF (${${EMBREE_ISA_NAME}} STREQUAL "AVX2")
    SET(EMBREE_ISA_SUPPORTS_AVX  TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX2 TRUE)
  ELSEIF (${${EMBREE_ISA_NAME}} STREQUAL "AVX512KNL")
    SET(EMBREE_ISA_SUPPORTS_AVX    TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX2   TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX512 TRUE)
  ENDIF()

  # Configure OSPRay ISA last after we've detected what we got w/ Embree
  OSPRAY_CONFIGURE_ISPC_ISA()

ELSE(OSPRAY_USE_EXTERNAL_EMBREE)

  # Clear out embree directories if they were previously populated by an
  # external find_package() call
  IF(${LAST_CONFIG_USED_EXTERNAL_EMBREE})
    UNSET(EMBREE_INCLUDE_DIRS)
    UNSET(EMBREE_LIBRARIES)
    UNSET(EMBREE_LIBRARY)
    UNSET(EMBREE_LIBRARY_XEONPHI)
  ENDIF()

  # Build Embree included in the OSPRay tree ##################################

  # NOTE(jda) - Embree assumes that OSPRAY_TASKING_TBB will be defined correctly
  #             in CONFIGURE_TASKING_SYSTEM()
  # NOTE(jda) - Only do the Embree include once (Xeon), it will build both
  #             Xeon and MIC code if both are enabled.
  IF (NOT THIS_IS_MIC)
    SET(EMBREE_ISA_SUPPORTS_AVX    TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX2   TRUE)
    SET(EMBREE_ISA_SUPPORTS_AVX512 TRUE)

    # Configure OSPRay ISA before building internal Embree so we know what ISA
    # for Embree to build.
    OSPRAY_CONFIGURE_ISPC_ISA()

    INCLUDE(build_embree)
  ENDIF()
  SET(EMBREE_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/ospray/embree-v2.7.1/include)
  SET(EMBREE_LIBRARY embree)
  SET(EMBREE_LIBRARIES ${EMBREE_LIBRARY})
  SET(EMBREE_LIBRARY_XEONPHI embree_xeonphi)
  SET(LAST_CONFIG_USED_EXTERNAL_EMBREE OFF CACHE INTERNAL "" FORCE)

ENDIF(OSPRAY_USE_EXTERNAL_EMBREE)

# NOTE(jda) - Windows not yet handled...
IF (OSPRAY_USE_EXTERNAL_EMBREE AND NOT WIN32 AND OSPRAY_ZIP_MODE)
  INSTALL(PROGRAMS ${EMBREE_LIBRARY}
          DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib) # /intel64?
  IF(OSPRAY_MIC)
    INSTALL(PROGRAMS ${EMBREE_LIBRARY_XEONPHI}
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/mic COMPONENT lib_mic)
  ENDIF()
ENDIF()
