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

CONFIGURE_OSPRAY()

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/apps/util)
INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES_ISPC(${PROJECT_SOURCE_DIR}/ospray)
INCLUDE_DIRECTORIES_ISPC(${EMBREE_DIR}/include)

IF (NOT THIS_IS_MIC)
  ADD_EXECUTABLE(ospParticleViewer
    ParticleViewer.cpp
    Model.cpp
    uintah.cpp
    scivis16.cpp
    )
  TARGET_LINK_LIBRARIES(ospParticleViewer
    ospray
    ospray_glut3d
    ospray_xml
    ${TBB_LIBRARY_MALLOC}
    ${TBB_LIBRARY}
  )
ENDIF()

