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

SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
SET(THREADS_PREFER_PTHREAD_FLAG TRUE)


IF (TARGET ospray_common)
  # this target is already built!
ELSE()
  FIND_PACKAGE(Threads REQUIRED)
  SET(LINK_LIBS ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${TASKING_SYSTEM_LIBS})

  IF (WIN32)
    LIST(APPEND LINK_LIBS ws2_32)
  ENDIF()

  OSPRAY_CREATE_LIBRARY(ospray_common
    common.cpp
    FileName.cpp
    sysinfo.cpp
    malloc.cpp
    library.cpp
    thread.cpp
    vec.cpp
    array3D/Array3D.cpp

    tasking/parallel_for.inl
    tasking/parallel_for.h
    tasking/async.h
    tasking/schedule.inl
    tasking/schedule.h
    tasking/tasking_system_handle.cpp
    tasking/TaskingTypeTraits.h
    tasking/TaskSys.cpp
    
    AffineSpace.h
    box.h
    constants.h
    intrinsics.h
    LinearSpace.h
    math.h
    platform.h
    Quaternion.h
    RefCount.h
    vec.h
  LINK
    ${LINK_LIBS}
  COMPONENT lib
  )

  OSPRAY_INSTALL_SDK_HEADERS(
    AffineSpace.h
    box.h
    common.h
    constants.h
    FileName.h
    intrinsics.h
    library.h
    LinearSpace.h
    malloc.h
    math.h
    platform.h
    Quaternion.h
    RefCount.h
    sysinfo.h
    thread.h
    vec.h

    DESTINATION ../ospcommon #NOTE: this is "next" to the SDK/ directory
    )

  OSPRAY_INSTALL_SDK_HEADERS(
    tasking/parallel_for.h
    tasking/async.h
    tasking/tasking_system_handle.h
    tasking/TaskSys.h
    tasking/TaskingTypeTraits.h

    DESTINATION ../ospcommon/tasking #NOTE: this is "next" to the SDK/ directory
    )
ENDIF()
