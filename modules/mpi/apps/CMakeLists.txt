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

# Test app which generates data-parallel groups of spheres ####################

include_directories(${CMAKE_SOURCE_DIR}/apps/exampleViewer)

if (NOT WIN32)# Internal Compiler Error on MSVC12...

  add_subdirectory(gensv)

  ospray_configure_mpi()

  ospray_create_test(ospRandSphereTest
    ospRandSphereTest.cpp
  LINK
    ospray
    ospray_mpi_common
  )

  ospray_create_test(ospRandSciVisTest
    ospRandSciVisTest.cpp
  LINK
    ospray
    ospray_mpi_common
    ospray_gensv
  )

  ospray_create_test(ospDDLoader-test
    ospDDLoader.cpp
  LINK
    ospray
    ospray_mpi_common
    ospray_tfn
    ospray_gensv
  )

  ospray_create_application(ospDDLoader
    ospDDLoader.cpp
  LINK
    ospray
    ospray_mpi_common
    ospray_tfn
    ospray_gensv
  )

  if (OSPRAY_MODULE_MPI_APPS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirectory(viewer)
  endif()

endif()
