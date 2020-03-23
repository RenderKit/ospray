## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include(GenerateExportHeader)

add_library(ospray_testing SHARED
  ${OSPRAY_RESOURCE}

  ospray_testing.cpp

  builders/Boxes.cpp
  builders/Builder.cpp
  builders/CornellBox.cpp
  builders/Curves.cpp
  builders/Empty.cpp
  builders/GravitySpheresVolume.cpp
  builders/PerlinNoiseVolumes.cpp
  builders/RandomSpheres.cpp
  builders/Streamlines.cpp
  builders/SubdivisionCube.cpp
  builders/UnstructuredVolume.cpp
  builders/Planes.cpp
  builders/ClippingGeometries.cpp

  # regression test scenes
  builders/test_pt_glass.cpp
  builders/test_pt_luminous.cpp
  builders/test_pt_metal_roughness.cpp
  builders/test_pt_metallic_flakes.cpp
  builders/test_pt_obj.cpp
)

generate_export_header(ospray_testing)

target_link_libraries(ospray_testing
PUBLIC
  ospray
  ospcommon::ospcommon
PRIVATE
  raw_to_amr
)

target_include_directories(ospray_testing
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/ospray_testing>
)

## Install library/headers ##

ospray_install_library(ospray_testing apps)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray
  COMPONENT devel
  FILES_MATCHING
  PATTERN *.h
  PATTERN *.inl
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ospray_testing_export.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/ospray_testing
  COMPONENT devel
)
