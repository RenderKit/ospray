## Copyright 2009-2020 Intel Corporation
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


set(OSPRAY_ISPC_DEVICE_SOURCES
  ${OSPRAY_RESOURCE}

  ispc_tasksys.cpp

  api/ISPCDevice.h
  api/ISPCDevice.cpp
  api/ISPCDevice.ispc

  common/Data.ih
  common/Data.cpp
  common/Future.cpp
  common/Group.cpp
  common/Group.ispc
  common/Instance.cpp
  common/Instance.ispc
  common/Managed.cpp
  common/OSPCommon.ispc
  common/World.ih
  common/World.ispc
  common/World.cpp

  fb/FrameBuffer.ispc
  fb/FrameBuffer.cpp
  fb/FrameBufferView.cpp
  fb/LocalFB.ispc
  fb/LocalFB.cpp
  fb/ImageOp.cpp
  fb/Tile.h
  fb/TileError.cpp
  fb/registration.cpp

  fb/frame_ops/Blur.cpp
  fb/frame_ops/Debug.cpp
  fb/frame_ops/Depth.cpp
  fb/frame_ops/SSAO.cpp
  fb/frame_ops/SSAO.ispc

  fb/tile_ops/SaveTiles.cpp
  fb/tile_ops/ToneMapper.cpp
  fb/tile_ops/ToneMapper.ispc

  pf/PixelFilter.ih
  pf/PixelFilter.ispc
  pf/LUTPixelFilter.ih
  pf/LUTPixelFilter.ispc
  pf/PixelFilter.cpp

  camera/Camera.cpp
  camera/Camera.ispc
  camera/PerspectiveCamera.ispc
  camera/PerspectiveCamera.cpp
  camera/OrthographicCamera.ispc
  camera/OrthographicCamera.cpp
  camera/PanoramicCamera.ispc
  camera/PanoramicCamera.cpp
  camera/registration.cpp

  geometry/Geometry.ispc
  geometry/Geometry.cpp
  geometry/GeometricModel.ispc
  geometry/GeometricModel.cpp
  geometry/Boxes.ispc
  geometry/Boxes.cpp
  geometry/Subdivision.ispc
  geometry/Subdivision.cpp
  geometry/Curves.cpp
  geometry/Curves.ispc
  geometry/Spheres.cpp
  geometry/Spheres.ispc
  geometry/Isosurfaces.ispc
  geometry/Isosurfaces.cpp
  geometry/Planes.ispc
  geometry/Planes.cpp
  geometry/Mesh.cpp
  geometry/Mesh.ispc
  geometry/registration.cpp

  lights/Light.ispc
  lights/Light.cpp
  lights/AmbientLight.ispc
  lights/AmbientLight.cpp
  lights/DirectionalLight.cpp
  lights/DirectionalLight.ispc
  lights/PointLight.cpp
  lights/PointLight.ispc
  lights/SpotLight.cpp
  lights/SpotLight.ispc
  lights/QuadLight.cpp
  lights/QuadLight.ispc
  lights/HDRILight.cpp
  lights/HDRILight.ispc
  lights/SunSkyLight.cpp
  lights/sky_model/sky_model.cpp
  lights/registration.cpp

  math/box.ispc
  math/Distribution1D.ispc
  math/Distribution2D.ispc
  math/sobol.ih
  math/sobol.ispc
  math/spectrum.h
  math/spectrum.ih
  math/spectrum.ispc
  math/random.ih
  math/halton.ih
  math/halton.ispc

  render/LoadBalancer.cpp
  render/Material.cpp
  render/Renderer.ispc
  render/Renderer.cpp
  render/util.ispc
  render/registration.cpp

  render/debug/DebugRenderer.cpp
  render/debug/DebugRenderer.ispc

  render/ao/AORenderer.cpp
  render/ao/AORenderer.ispc
  render/ao/Material.cpp
  render/ao/Material.ispc
  render/ao/surfaces.ispc
  render/ao/volumes.ispc

  render/scivis/SciVis.cpp
  render/scivis/SciVis.ispc
  render/scivis/SciVisMaterial.cpp
  render/scivis/SciVisMaterial.ispc
  render/scivis/surfaces.ispc
  render/scivis/volumes.ispc
  render/scivis/lightAlpha.ispc

  render/pathtracer/PathTracer.ispc
  render/pathtracer/PathTracer.cpp
  render/pathtracer/PathSampler.ispc
  render/pathtracer/GeometryLight.ispc
  render/pathtracer/VirtualLight.ispc
  render/pathtracer/TransparentShadow.ispc
  render/pathtracer/ShadowCatcher.ispc
  render/pathtracer/NextEventEstimation.ispc
  render/pathtracer/bsdfs/MicrofacetAlbedoTables.ispc
  render/pathtracer/volumes/VolumeSampler.ispc
  render/pathtracer/materials/Material.ispc
  render/pathtracer/materials/OBJ.ispc
  render/pathtracer/materials/OBJ.cpp
  render/pathtracer/materials/Principled.ispc
  render/pathtracer/materials/Principled.cpp
  render/pathtracer/materials/CarPaint.ispc
  render/pathtracer/materials/CarPaint.cpp
  render/pathtracer/materials/Velvet.ispc
  render/pathtracer/materials/Velvet.cpp
  render/pathtracer/materials/Metal.ispc
  render/pathtracer/materials/Metal.cpp
  render/pathtracer/materials/Alloy.ispc
  render/pathtracer/materials/Alloy.cpp
  render/pathtracer/materials/ThinGlass.ispc
  render/pathtracer/materials/ThinGlass.cpp
  render/pathtracer/materials/Glass.ispc
  render/pathtracer/materials/Glass.cpp
  render/pathtracer/materials/MetallicPaint.ispc
  render/pathtracer/materials/MetallicPaint.cpp
  render/pathtracer/materials/Plastic.ispc
  render/pathtracer/materials/Plastic.cpp
  render/pathtracer/materials/Luminous.ispc
  render/pathtracer/materials/Luminous.cpp
  render/pathtracer/materials/Mix.ispc
  render/pathtracer/materials/Mix.cpp

  texture/Texture.cpp
  texture/Texture2D.cpp
  texture/Texture2D.ispc
  texture/TextureVolume.cpp
  texture/TextureVolume.ispc
  texture/TextureParam.ispc
  texture/registration.cpp

  volume/Volume.ispc
  volume/Volume.cpp
  volume/VolumetricModel.ispc
  volume/VolumetricModel.cpp

  volume/transferFunction/LinearTransferFunction.ispc
  volume/transferFunction/LinearTransferFunction.cpp
  volume/transferFunction/TransferFunction.ispc
  volume/transferFunction/TransferFunction.cpp
  volume/transferFunction/registration.cpp
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
      PATTERN *.ih
    )
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dir}
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK
      COMPONENT devel
      FILES_MATCHING
      PATTERN *_ispc.h
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
# Build ISPC device
##############################################################

option(OSPRAY_BUILD_ISPC_DEVICE "Build ISPC Device" ON)
mark_as_advanced(OSPRAY_BUILD_ISPC_DEVICE)

if(OSPRAY_BUILD_ISPC_DEVICE)

  ## Windows-specifc ISPC symbol exports ##

  if (WIN32)
    file(READ def_header.txt OSPRAY_DEF)
    file(READ ispc_symbols.txt OSPRAY_ISPC_SYMBOLS_IN)

    foreach(isa ${OSPRAY_ISPC_TARGET_LIST})
      string(REPLACE "-i32x16" "" isa ${isa}) # strip avx512(knl|skx)-i32x16
      # add isa suffix
      string(REPLACE "," ${isa} OSPRAY_ISPC_SYMBOLS ${OSPRAY_ISPC_SYMBOLS_IN})
      string(APPEND OSPRAY_DEF ${OSPRAY_ISPC_SYMBOLS})
    endforeach()

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ospray_module_ispc.def.in "${OSPRAY_DEF}")

    # changes ospray.def only if content changed, avoids unnecessary re-linking
    configure_file(${CMAKE_CURRENT_BINARY_DIR}/ospray_module_ispc.def.in ospray_module_ispc.def COPYONLY)

    list(APPEND OSPRAY_ISPC_DEVICE_SOURCES ospray_module_ispc.def)
  endif()

  ## ISPC Device-specific options ##

  set(OSPRAY_TILE_SIZE 64 CACHE STRING "Tile size (x,y dimensions)")
  set_property(CACHE OSPRAY_TILE_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)
  mark_as_advanced(OSPRAY_TILE_SIZE)

  if (WIN32)
    set(TILE_STACK_DEFAULT_SIZE 64)
  elseif (APPLE)
    set(TILE_STACK_DEFAULT_SIZE 32)
  else ()
    set(TILE_STACK_DEFAULT_SIZE 128)
  endif()

  set(OSPRAY_MAX_STACK_TILE_SIZE ${TILE_STACK_DEFAULT_SIZE} CACHE STRING
      "Max size for tile to remain allocated on the stack")
  set_property(CACHE OSPRAY_MAX_STACK_TILE_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)
  mark_as_advanced(OSPRAY_MAX_STACK_TILE_SIZE)

  set(OSPRAY_PIXELS_PER_JOB 64 CACHE STRING
      "Must be multiple of largest vector width *and* <= OSPRAY_TILE_SIZE")
  mark_as_advanced(OSPRAY_PIXELS_PER_JOB)

  option(OSPRAY_DEBUG_NO_TASKING
         "Remove tasking parallelism system for debugging"
         OFF)
  mark_as_advanced(OSPRAY_DEBUG_NO_TASKING)

  configure_file(common/OSPConfig.h.in ../OSPConfig.h)
  install(FILES ${CMAKE_BINARY_DIR}/OSPConfig.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK
    COMPONENT devel
  )

  ## Build the module ##

  ispc_include_directories(
    ${CMAKE_SOURCE_DIR}/ospray/include
    ${CMAKE_SOURCE_DIR}/ospray
    ${CMAKE_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
    ${EMBREE_INCLUDE_DIRS}
    ${OPENVKL_INCLUDE_DIRS}
  )
  add_library(ospray_module_ispc SHARED)

  ispc_target_add_sources(ospray_module_ispc ${OSPRAY_ISPC_DEVICE_SOURCES})

  ospray_install_library(ospray_module_ispc lib)

  target_link_libraries(ospray_module_ispc
  PUBLIC
    ospray
    rkcommon::rkcommon
    $<BUILD_INTERFACE:embree>
    $<BUILD_INTERFACE:openvkl::openvkl>
  )

  target_include_directories(ospray_module_ispc
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
      # NOTE(jda) - the following includes are missing despite PUBLIC linking
      $<BUILD_INTERFACE:${OPENVKL_INCLUDE_DIRS}>
      $<BUILD_INTERFACE:${EMBREE_INCLUDE_DIRS}>
      #######################################################################
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_compile_definitions(ospray_module_ispc
  PUBLIC
    TILE_SIZE=${OSPRAY_TILE_SIZE}
    MAX_TILE_SIZE=${OSPRAY_MAX_STACK_TILE_SIZE}
    RENDERTILE_PIXELS_PER_JOB=${OSPRAY_PIXELS_PER_JOB}
    $<$<BOOL:${OSPRAY_DEBUG_NO_TASKING}>:OSPRAY_SERIAL_RENDERING>
  )

endif()

##############################################################
# Additional interface targets
##############################################################

add_library(ospray_sdk INTERFACE)
target_link_libraries(ospray_sdk
INTERFACE
  ospray_module_ispc rkcommon::rkcommon
)
ospray_install_target(ospray_sdk devel)

##############################################################
# Configure find_package files
##############################################################

include(${CMAKE_SOURCE_DIR}/cmake/ospray_cmake_config.cmake)
