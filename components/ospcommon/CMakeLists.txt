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

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

if (TARGET ospray_common)
  # this target is already built!
else()
  find_package(Threads REQUIRED)
  set(LINK_LIBS ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${TASKING_SYSTEM_LIBS})

  if (WIN32)
    list(APPEND LINK_LIBS ws2_32)
  endif()

  ## Create ospcommon library ##

  ospray_create_library(ospray_common
    common.cpp
    FileName.cpp
    sysinfo.cpp
    library.cpp
    thread.cpp
    vec.cpp

    array3D/Array3D.cpp

    containers/TransactionalBuffer.h

    memory/DeletedUniquePtr.h
    memory/malloc.cpp
    memory/RefCount.h

    networking/BufferedDataStreaming.cpp
    networking/DataStreaming.h
    networking/Fabric.h
    networking/Socket.cpp
    networking/SocketFabric.cpp

    tasking/detail/parallel_for.inl
    tasking/detail/schedule.inl
    tasking/detail/tasking_system_handle.cpp
    tasking/detail/TaskSys.cpp
    tasking/async.h
    tasking/parallel_for.h
    tasking/schedule.h

    utility/Any.h
    utility/CodeTimer.h
    utility/DataView.h
    utility/demangle.cpp
    utility/DoubleBufferedValue.h
    utility/getEnvVar.h
    utility/OnScopeExit.h
    utility/Optional.h
    utility/ParameterizedObject.cpp
    utility/PseudoURL.cpp
    utility/SaveImage.h
    utility/StringManip.h
    utility/TimeStamp.cpp
    utility/TransactionalValue.h

    xml/XML.cpp

    AffineSpace.h
    box.h
    constants.h
    intrinsics.h
    LinearSpace.h
    math.h
    platform.h
    Quaternion.h
    TypeTraits.h
    vec.h
  LINK
    ${LINK_LIBS}
  COMPONENT lib
  )

  ## Install headers ##

  set(OSPCOMMON_SDK_INSTALL_LOC
    ../ospcommon #NOTE: this is "next" to the SDK/ directory
  )

  ospray_install_sdk_headers(
    AffineSpace.h
    AsyncLoop.h
    box.h
    common.h
    constants.h
    FileName.h
    intrinsics.h
    library.h
    LinearSpace.h
    math.h
    multidim_index_sequence.h
    platform.h
    Quaternion.h
    range.h
    sysinfo.h
    thread.h
    TypeTraits.h
    vec.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}
  )

  ospray_install_sdk_headers(
    array3D/Array3D.h
    array3D/for_each.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/array3D
  )

  ospray_install_sdk_headers(
    containers/aligned_allocator.h
    containers/AlignedVector.h
    containers/TransactionalBuffer.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/containers
  )

  ospray_install_sdk_headers(
    memory/DeletedUniquePtr.h
    memory/malloc.h
    memory/RefCount.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/memory
  )

  ospray_install_sdk_headers(
    networking/BufferedDataStreaming.h
    networking/DataStreaming.h
    networking/Fabric.h
    networking/Socket.h
    networking/SocketFabric.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/networking
  )

  ospray_install_sdk_headers(
    tasking/async.h
    tasking/parallel_for.h
    tasking/parallel_foreach.h
    tasking/schedule.h
    tasking/tasking_system_handle.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/tasking
  )

  ospray_install_sdk_headers(
    tasking/detail/parallel_for.inl
    tasking/detail/schedule.inl
    tasking/detail/TaskSys.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/tasking/detail
  )

  ospray_install_sdk_headers(
    utility/Any.h
    utility/ArgumentList.h
    utility/ArrayView.h
    utility/CodeTimer.h
    utility/DataView.h
    utility/demangle.h
    utility/DoubleBufferedValue.h
    utility/getEnvVar.h
    utility/Observer.h
    utility/OnScopeExit.h
    utility/Optional.h
    utility/ParameterizedObject.h
    utility/PseudoURL.h
    utility/SaveImage.h
    utility/StringManip.h
    utility/TimeStamp.h
    utility/TransactionalValue.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/utility
  )

  ospray_install_sdk_headers(
    xml/XML.h

    DESTINATION ${OSPCOMMON_SDK_INSTALL_LOC}/xml
  )

  ## Build Test Apps ##

  if (OSPRAY_ENABLE_TESTING)
    add_library(ospray_test_main STATIC testing/catch_main.cpp)
  endif()

  # containers/AlignedVector
  ospray_create_test(test_AlignedVector
    containers/tests/test_AlignedVector.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # containers/TransactionalBuffer
  ospray_create_test(test_TransactionalBuffer
    containers/tests/test_TransactionalBuffer.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # tasking/async
  ospray_create_test(test_async
    tasking/tests/test_async.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # tasking/parallel_for
  ospray_create_test(test_parallel_for
    tasking/tests/test_parallel_for.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # tasking/parallel_foreach
  ospray_create_test(test_parallel_foreach
    tasking/tests/test_parallel_foreach.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # tasking/schedule
  ospray_create_test(test_schedule
    tasking/tests/test_schedule.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # utility/Any
  ospray_create_test(test_Any
    utility/tests/test_Any.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # utility/ArgumentList
  ospray_create_test(test_ArgumentList
    utility/tests/test_ArgumentList.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # utility/ArrayView
  ospray_create_test(test_ArrayView
    utility/tests/test_ArrayView.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # utility/Observers
  ospray_create_test(test_Observers
    utility/tests/test_Observers.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # utility/OnScopeExit
  ospray_create_test(test_OnScopeExit
    utility/tests/test_OnScopeExit.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # utility/Optional
  ospray_create_test(test_Optional
    utility/tests/test_Optional.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # utility/ParameterizedObject
  ospray_create_test(test_ParameterizedObject
    utility/tests/test_ParameterizedObject.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  # utility/StringManip
  ospray_create_test(test_StringManip
    utility/tests/test_StringManip.cpp
  LINK
    ospray_common
    ospray_test_main
  )

  ## Add Test Apps to CTest ##

  add_test(NAME ArgumentList          COMMAND test_ArgumentList       )
  add_test(NAME ArrayView             COMMAND test_ArrayView          )
  add_test(NAME OnScopeExit           COMMAND test_OnScopeExit        )
  add_test(NAME Optional              COMMAND test_Optional           )
  add_test(NAME TransactionalBuffer   COMMAND test_TransactionalBuffer)
  add_test(NAME StringManip           COMMAND test_StringManip        )
  if(NOT WIN32) # Tests which are broken on Windows with unknown fixes (for now)
    add_test(NAME Any                 COMMAND test_Any                )
    add_test(NAME AlignedVector       COMMAND test_AlignedVector      )
    add_test(NAME Observers           COMMAND test_Observers          )
    add_test(NAME ParameterizedObject COMMAND test_ParameterizedObject)
    add_test(NAME async               COMMAND test_async              )
    add_test(NAME parallel_for        COMMAND test_parallel_for       )
    add_test(NAME parallel_foreach    COMMAND test_parallel_foreach   )
    add_test(NAME schedule            COMMAND test_schedule           )
  endif()
endif()
