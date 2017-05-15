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

  ## Create ospcommon library ##

  OSPRAY_CREATE_LIBRARY(ospray_common
    common.cpp
    FileName.cpp
    sysinfo.cpp
    malloc.cpp
    library.cpp
    thread.cpp
    vec.cpp
    array3D/Array3D.cpp

    containers/TransactionalBuffer.h

    networking/BufferedDataStreaming.cpp
    networking/DataStreaming.h
    networking/Fabric.h
    networking/Socket.cpp

    tasking/parallel_for.inl
    tasking/parallel_for.h
    tasking/async.h
    tasking/schedule.inl
    tasking/schedule.h
    tasking/tasking_system_handle.cpp
    tasking/TaskSys.cpp

    utility/Any.h
    utility/FPSCounter.h
    utility/OnScopeExit.h
    utility/PseudoURL.cpp
    utility/TransactionalValue.h

    AffineSpace.h
    box.h
    constants.h
    intrinsics.h
    LinearSpace.h
    math.h
    platform.h
    Quaternion.h
    RefCount.h
    TypeTraits.h
    vec.h
  LINK
    ${LINK_LIBS}
  COMPONENT lib
  )

  ## Install headers ##

  SET(OSPCOMMON_SDK_INSTALL_LOC
    ../ospcommon #NOTE: this is "next" to the SDK/ directory
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
    TypeTraits.h
    vec.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}
  )

  OSPRAY_INSTALL_SDK_HEADERS(
    containers/TransactionalBuffer.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/containers
  )

  OSPRAY_INSTALL_SDK_HEADERS(
    networking/BufferedDataStreaming.h
    networking/DataStreaming.h
    networking/Fabric.h
    networking/Socket.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/networking
  )

  OSPRAY_INSTALL_SDK_HEADERS(
    tasking/parallel_for.h
    tasking/async.h
    tasking/tasking_system_handle.h
    tasking/TaskSys.h
    tasking/TaskingTypeTraits.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/tasking
  )

  OSPRAY_INSTALL_SDK_HEADERS(
    utility/Any.h
    utility/FPSCounter.h
    utility/OnScopeExit.h
    utility/PseudoURL.h
    utility/TransactionalValue.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/utility
  )

  ## Test apps ##

  # Any

  OSPRAY_CREATE_TEST(test_Any
    utility/Any.h
    utility/test_Any.cpp
  )

  ADD_TEST(NAME Any COMMAND test_Any)

  # OnScopeExit

  OSPRAY_CREATE_TEST(test_OnScopeExit
    utility/OnScopeExit.h
    utility/test_OnScopeExit.cpp
  )

  ADD_TEST(NAME OnScopeExit COMMAND test_OnScopeExit)

  # TransactionalBuffer

  OSPRAY_CREATE_TEST(test_TransactionalBuffer
    containers/TransactionalBuffer.h
    containers/test_TransactionalBuffer.cpp
  )

  ADD_TEST(NAME TransactionalBuffer COMMAND test_TransactionalBuffer)
ENDIF()
