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

OPTION(OSPRAY_DISPLAYCLUSTER "Build DisplayCluster tiled display support")
MARK_AS_ADVANCED(OSPRAY_DISPLAYCLUSTER)

IF (OSPRAY_DISPLAYCLUSTER)

  FIND_PATH(DISPLAYCLUSTER_INCLUDE_DIR dcStream.h
    $ENV{DISPLAYCLUSTER_DIR}/include
    )

  FIND_PATH(DISPLAYCLUSTER_LINK_DIR libDisplayCluster.so
    $ENV{DISPLAYCLUSTER_DIR}/lib
    )

  FIND_LIBRARY(DISPLAYCLUSTER_LIBRARY NAMES libDisplayCluster.so PATHS $ENV{DISPLAYCLUSTER_DIR}/lib ${DISPLAYCLUSTER_LINK_DIR})

  SET(DISPLAYCLUSTER_LIBRARIES ${DISPLAYCLUSTER_LIBRARY})

  MACRO(CONFIGURE_DISPLAYCLUSTER)
    INCLUDE_DIRECTORIES(${DISPLAYCLUSTER_INCLUDE_DIR})
    LINK_DIRECTORIES(${DISPLAYCLUSTER_LINK_DIR})
  ENDMACRO()

ENDIF (OSPRAY_DISPLAYCLUSTER)
