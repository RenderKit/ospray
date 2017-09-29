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

# Test app which generates data-parallel groups of spheres ####################

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/apps/exampleViewer)

IF (NOT WIN32)# Internal Compiler Error on MSVC12...
  ADD_SUBDIRECTORY(gensv)

  OSPRAY_CREATE_TEST(ospRandSphereTest
    ospRandSphereTest.cpp
  LINK
    ospray
    ospray_mpi_common
  )

  OSPRAY_CREATE_TEST(ospRandSciVisTest
    ospRandSciVisTest.cpp
  LINK
    ospray
    ospray_mpi_common
    gensv
  )

  OSPRAY_CREATE_TEST(ospDDLoader
    ospDDLoader.cpp
  LINK
    ospray
    ospray_mpi_common
    ospray_tfn
    gensv
  )

  OPTION(OSPRAY_MODULE_MPI_APPS "MPI module viewer application"
    ${OSPRAY_APPS_EXAMPLEVIEWER})

  IF (OSPRAY_MODULE_MPI_APPS)
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
    ADD_SUBDIRECTORY(viewer)
  ENDIF()
ENDIF()
