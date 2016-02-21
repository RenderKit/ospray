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

IF (NOT THIS_IS_MIC)
  SET(USE_IMAGE_MAGICK OFF CACHE BOOL "Use ImageMagick for ModelViewer's Texture loaders.")
  MARK_AS_ADVANCED(USE_IMAGE_MAGICK)
  IF (USE_IMAGE_MAGICK)
    IF (APPLE)
      MESSAGE(ERROR "ImageMagick currently not supported under Apple")
      # the magick libraries apparently have some issues on mac that
      # requrie ospray to be built with very special compile flags; we
      # can specify those, but then it no longer works with the
      # mac-version of Qt, so we currently pick qt compatibliity over
      # magick compatibility.
    ELSE(APPLE)
      FIND_PACKAGE(ImageMagick COMPONENTS Magick++)
      IF (IMAGEMAGICK_FOUND)
        ADD_DEFINITIONS(-DUSE_IMAGEMAGICK)
        INCLUDE_DIRECTORIES(${ImageMagick_Magick++_INCLUDE_DIR})
        SET(MAGICK_LIBRARIES ${ImageMagick_Magick++_LIBRARY})
      ELSE (IMAGEMAGICK_FOUND)
        MESSAGE(STATUS "ImageMagick not found. Texture loaders other than PPM are disabled.")
      ENDIF (IMAGEMAGICK_FOUND)
    ENDIF(APPLE)
  ENDIF (USE_IMAGE_MAGICK)

  INCLUDE_DIRECTORIES(${OSPRAY_DIR}/ospray)
  
  INCLUDE_DIRECTORIES(${OSPRAY_DIR})

  ADD_LIBRARY(ospray_minisg 
    miniSG.cpp
    importer.cpp
    importOBJ.cpp
    importHBP.cpp
    importSTL.cpp
    importMSG.cpp
    importTRI.cpp
    importX3D.cpp
    importRIVL.cpp
    )
  TARGET_LINK_LIBRARIES(ospray_minisg ospray_xml
    ${MAGICK_LIBRARIES})
  # ------------------------------------------------------------
  SET_TARGET_PROPERTIES(ospray_minisg${OSPRAY_LIB_SUFFIX} 
    PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION  ${OSPRAY_SOVERSION})
  INSTALL(TARGETS ospray_minisg${OSPRAY_LIB_SUFFIX} DESTINATION lib)
  # ------------------------------------------------------------

ENDIF(NOT THIS_IS_MIC)
