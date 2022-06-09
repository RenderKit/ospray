## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# -------------------------------------------------------
# Setup ospray include directories and source files
# -------------------------------------------------------

set(OSPRAY_RESOURCE ${CMAKE_CURRENT_SOURCE_DIR}/ospray.rc)
set(OSPRAY_RESOURCE ${OSPRAY_RESOURCE} PARENT_SCOPE)

set(OSPRAY_SOURCES
  ${OSPRAY_RESOURCE}

  api/API.cpp
  api/Device.cpp
  api/ospray_util_impl.cpp
  api/ospray_cpp_defs.cpp

  common/OSPCommon.cpp

  include/ospray/ospray.h
  include/ospray/ospray_util.h
  include/ospray/OSPEnums.h

  include/ospray/ospray_cpp.h
  include/ospray/ospray_cpp/Camera.h
  include/ospray/ospray_cpp/Data.h
  include/ospray/ospray_cpp/FrameBuffer.h
  include/ospray/ospray_cpp/Geometry.h
  include/ospray/ospray_cpp/Light.h
  include/ospray/ospray_cpp/ManagedObject.h
  include/ospray/ospray_cpp/Material.h
  include/ospray/ospray_cpp/ImageOperation.h
  include/ospray/ospray_cpp/Renderer.h
  include/ospray/ospray_cpp/Texture.h
  include/ospray/ospray_cpp/TransferFunction.h
  include/ospray/ospray_cpp/Volume.h
  include/ospray/ospray_cpp/World.h
)

# -------------------------------------------------------
# Install SDK headers
# -------------------------------------------------------

get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

foreach(dir ${SUBDIRS})
  if (NOT "${dir}" STREQUAL include)
    install(DIRECTORY ${dir}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK
      COMPONENT devel
      FILES_MATCHING
      PATTERN *.h
    )
  endif()
endforeach()

##############################################################
# Build the ospray library
##############################################################

add_library(ospray SHARED ${OSPRAY_SOURCES})
target_link_libraries(ospray PUBLIC $<BUILD_INTERFACE:rkcommon::rkcommon>)
ospray_install_library(ospray lib)

target_include_directories(ospray
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

##############################################################
# Configure find_package files
##############################################################

include(${CMAKE_SOURCE_DIR}/cmake/ospray_cmake_config.cmake)
