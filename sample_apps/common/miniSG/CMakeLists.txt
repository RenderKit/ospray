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

set(LIBRARY_NAME ospray_minisg)

set(USE_IMAGE_MAGICK OFF CACHE BOOL "Use ImageMagick for ModelViewer's Texture loaders.")
mark_as_advanced(USE_IMAGE_MAGICK)
if (USE_IMAGE_MAGICK)
  if (APPLE)
    message(ERROR "ImageMagick currently not supported under Apple")
    # the magick libraries apparently have some issues on mac that
    # requrie ospray to be built with very special compile flags; we
    # can specify those, but then it no longer works with the
    # mac-version of Qt, so we currently pick qt compatibliity over
    # magick compatibility.
  else(APPLE)
    find_package(ImageMagick COMPONENTS Magick++)
    if (IMAGEMAGICK_FOUND)
      add_definitions(-DUSE_IMAGEMAGICK)
      include_directories(${ImageMagick_Magick++_INCLUDE_DIR})
      set(MAGICK_LIBRARIES ${ImageMagick_Magick++_LIBRARY})
    else (IMAGEMAGICK_FOUND)
      message(STATUS "ImageMagick not found. Texture loaders other than PPM are disabled.")
    endif (IMAGEMAGICK_FOUND)
  endif(APPLE)
endif (USE_IMAGE_MAGICK)

add_library(${LIBRARY_NAME}
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

target_link_libraries(${LIBRARY_NAME}
  ospray_xml
  ospray_common
  ospray
  ${MAGICK_LIBRARIES}
)

ospray_set_library_version(${LIBRARY_NAME})

install(TARGETS ${LIBRARY_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT devel
)
