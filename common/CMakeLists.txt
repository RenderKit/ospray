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

SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
FIND_PACKAGE(Threads REQUIRED)

OSPRAY_ADD_LIBRARY(ospray_common SHARED
  common.cpp
  FileName.cpp
  sysinfo.cpp
  malloc.cpp
  library.cpp
  thread.cpp
  vec.cpp
  network.cpp

  AffineSpace.h
  box.h
  constants.h
  intrinsics.h
  LinearSpace.h
  math.h
  mpi.h
  platform.h
  Quaternion.h
  RefCount.h
  vec.h
)

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_common
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)
IF (WIN32)
  OSPRAY_LIBRARY_LINK_LIBRARIES(ospray_common ws2_32)
ENDIF()

OSPRAY_SET_LIBRARY_VERSION(ospray_common)
OSPRAY_INSTALL_LIBRARY(ospray_common)
