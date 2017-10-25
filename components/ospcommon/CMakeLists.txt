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

    tasking/detail/parallel_for.inl
    tasking/detail/schedule.inl
    tasking/detail/tasking_system_handle.cpp
    tasking/detail/TaskSys.cpp
    tasking/async.h
    tasking/parallel_for.h
    tasking/schedule.h

    utility/Any.h
    utility/CodeTimer.h
    utility/DoubleBufferedValue.h
    utility/OnScopeExit.h
    utility/Optional.h
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
    AsyncLoop.h
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
    range.h
    RefCount.h
    sysinfo.h
    thread.h
    TypeTraits.h
    vec.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}
  )

  OSPRAY_INSTALL_SDK_HEADERS(
    array3D/Array3D.h
    array3D/for_each.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/array3D
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
    tasking/async.h
    tasking/parallel_for.h
    tasking/schedule.h
    tasking/tasking_system_handle.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/tasking
  )

  OSPRAY_INSTALL_SDK_HEADERS(
    tasking/detail/parallel_for.inl
    tasking/detail/schedule.inl
    tasking/detail/TaskSys.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/tasking/detail
  )

  OSPRAY_INSTALL_SDK_HEADERS(
    utility/Any.h
    utility/CodeTimer.h
    utility/DoubleBufferedValue.h
    utility/OnScopeExit.h
    utility/Optional.h
    utility/PseudoURL.h
    utility/TransactionalValue.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/utility
  )

  ## Test apps ##

  # Any

  OSPRAY_CREATE_TEST(test_Any
    utility/tests/test_Any.cpp
  LINK
    ospray_common
  )

  ADD_TEST(NAME Any COMMAND test_Any)

  # ArrayView

  OSPRAY_CREATE_TEST(test_ArrayView
    utility/tests/test_ArrayView.cpp
  LINK
    ospray_common
  )

  ADD_TEST(NAME ArrayView COMMAND test_ArrayView)

  # utility/OnScopeExit

  OSPRAY_CREATE_TEST(test_OnScopeExit
    utility/tests/test_OnScopeExit.cpp
  LINK
    ospray_common
  )

  ADD_TEST(NAME OnScopeExit COMMAND test_OnScopeExit)

  # Optional

  OSPRAY_CREATE_TEST(test_Optional
    utility/tests/test_Optional.cpp
  LINK
    ospray_common
  )

  ADD_TEST(NAME Optional COMMAND test_Optional)

  # containers/TransactionalBuffer

  OSPRAY_CREATE_TEST(test_TransactionalBuffer
    containers/tests/test_TransactionalBuffer.cpp
  LINK
    ospray_common
  )

  ADD_TEST(NAME TransactionalBuffer COMMAND test_TransactionalBuffer)

  IF(NOT WIN32)
    # tasking/async
    OSPRAY_CREATE_TEST(test_async
      tasking/tests/test_async.cpp
    LINK
      ospray_common
    )

    ADD_TEST(NAME async COMMAND test_async)

    # tasking/parallel_for
    OSPRAY_CREATE_TEST(test_parallel_for
      tasking/tests/test_parallel_for.cpp
    LINK
      ospray_common
    )

    ADD_TEST(NAME parallel_for COMMAND test_parallel_for)

    # tasking/schedule
    OSPRAY_CREATE_TEST(test_schedule
      tasking/tests/test_schedule.cpp
    LINK
      ospray_common
    )

    ADD_TEST(NAME schedule COMMAND test_schedule)
  ENDIF()

ENDIF()
