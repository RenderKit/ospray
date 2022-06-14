## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_dependent_option(
  OSPRAY_MODULE_CPU
  "Build the CPU module"
  ON
  OSPRAY_ENABLE_MODULES
  OFF
)
mark_as_advanced(OSPRAY_MODULE_CPU)

if (NOT OSPRAY_MODULE_CPU)
  return()
endif()

# -------------------------------------------------------
# Setup module include directories and source files
# -------------------------------------------------------

set(OSPRAY_ISPC_DEVICE_SOURCES
  ${OSPRAY_RESOURCE}

  ispc_tasksys.cpp

  ISPCDevice.h
  ISPCDevice.cpp
  ISPCDevice.ispc

  common/Data.ih
  common/Data.cpp
  common/Future.cpp
  common/Group.cpp
  common/Instance.cpp
  common/MotionTransform.cpp
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
  fb/SparseFB.cpp
  fb/SparseFB.ispc
  fb/ImageOp.cpp
  fb/TaskError.cpp
  fb/registration.cpp

  fb/frame_ops/Blur.cpp
  fb/frame_ops/Debug.cpp
  fb/frame_ops/Depth.cpp
  fb/frame_ops/SSAO.cpp
  fb/frame_ops/SSAO.ispc

  fb/pixel_ops/ToneMapper.cpp
  fb/pixel_ops/ToneMapper.ispc

  pf/PixelFilter.ih
  pf/PixelFilter.ispc
  pf/LUTPixelFilter.ih
  pf/LUTPixelFilter.ispc
  pf/PixelFilter.cpp

  camera/Camera.cpp
  camera/PerspectiveCamera.ispc
  camera/PerspectiveCamera.cpp
  camera/OrthographicCamera.ispc
  camera/OrthographicCamera.cpp
  camera/PanoramicCamera.ispc
  camera/PanoramicCamera.cpp
  camera/registration.cpp

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
  lights/IntensityDistribution.cpp
  lights/SpotLight.cpp
  lights/SpotLight.ispc
  lights/QuadLight.cpp
  lights/QuadLight.ispc
  lights/CylinderLight.cpp
  lights/CylinderLight.ispc
  lights/HDRILight.cpp
  lights/HDRILight.ispc
  lights/SunSkyLight.cpp
  lights/sky_model/sky_model.cpp
  lights/registration.cpp

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
  render/Material.ispc
  render/Renderer.ispc
  render/Renderer.cpp
  render/util.ispc
  render/registration.cpp

  render/debug/DebugRenderer.cpp
  render/debug/DebugRenderer.ispc

  render/ao/AORenderer.cpp
  render/ao/AORenderer.ispc
  render/ao/surfaces.ispc
  render/ao/volumes.ispc

  render/scivis/SciVis.cpp
  render/scivis/SciVis.ispc
  render/scivis/SciVisData.cpp
  render/scivis/surfaces.ispc
  render/scivis/volumes.ispc
  render/scivis/lightAlpha.ispc

  render/pathtracer/PathTracer.ispc
  render/pathtracer/PathTracer.cpp
  render/pathtracer/PathTracerData.cpp
  render/pathtracer/PathSampler.ispc
  render/pathtracer/GeometryLight.ispc
  render/pathtracer/GeometryLight.cpp
  render/pathtracer/VirtualLight.ispc
  render/pathtracer/TransparentShadow.ispc
  render/pathtracer/ShadowCatcher.ispc
  render/pathtracer/NextEventEstimation.ispc
  render/pathtracer/volumes/VolumeSampler.ispc

  render/bsdfs/MicrofacetAlbedoTables.ispc
  render/materials/OBJ.ispc
  render/materials/OBJ.cpp
  render/materials/Principled.ispc
  render/materials/Principled.cpp
  render/materials/CarPaint.ispc
  render/materials/CarPaint.cpp
  render/materials/Velvet.ispc
  render/materials/Velvet.cpp
  render/materials/Metal.ispc
  render/materials/Metal.cpp
  render/materials/Alloy.ispc
  render/materials/Alloy.cpp
  render/materials/ThinGlass.ispc
  render/materials/ThinGlass.cpp
  render/materials/Glass.ispc
  render/materials/Glass.cpp
  render/materials/MetallicPaint.ispc
  render/materials/MetallicPaint.cpp
  render/materials/Plastic.ispc
  render/materials/Plastic.cpp
  render/materials/Luminous.ispc
  render/materials/Luminous.cpp
  render/materials/Mix.ispc
  render/materials/Mix.cpp

  texture/Texture.cpp
  texture/Texture2D.cpp
  texture/Texture2D.ispc
  texture/TextureVolume.cpp
  texture/TextureVolume.ispc
  texture/registration.cpp

  volume/Volume.ispc
  volume/Volume.cpp
  volume/VolumetricModel.cpp

  volume/transferFunction/LinearTransferFunction.ispc
  volume/transferFunction/LinearTransferFunction.cpp
  volume/transferFunction/TransferFunction.cpp
  volume/transferFunction/registration.cpp
)

##############################################################
# Build ISPC device
##############################################################


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

  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ospray_module_cpu.def.in "${OSPRAY_DEF}")

  # changes .def only if content changed, avoids unnecessary re-linking
  configure_file(${CMAKE_CURRENT_BINARY_DIR}/ospray_module_cpu.def.in ospray_module_cpu.def COPYONLY)

  list(APPEND OSPRAY_ISPC_DEVICE_SOURCES ospray_module_cpu.def)
endif()

## ISPC Device-specific options ##

set(OSPRAY_TILE_SIZE 64 CACHE STRING "Tile size (x,y dimensions)")
set_property(CACHE OSPRAY_TILE_SIZE PROPERTY STRINGS 8 16 32 64 128 256 512)
mark_as_advanced(OSPRAY_TILE_SIZE)

set(OSPRAY_RENDER_TASK_SIZE 8 CACHE STRING
    "Render task size (x,y dimensions). 8 is the default, -1 indicates to use a SIMD-width render task size determined at runtime. Must be less than OSPRAY_TILE_SIZE.")
set_property(CACHE OSPRAY_RENDER_TASK_SIZE PROPERTY STRINGS -1 2 4 8 16 32 64)
mark_as_advanced(OSPRAY_RENDER_TASK_SIZE)

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

option(OSPRAY_DEBUG_NO_TASKING
        "Remove tasking parallelism system for debugging"
        OFF)
mark_as_advanced(OSPRAY_DEBUG_NO_TASKING)

configure_file(common/OSPConfig.h.in ${PROJECT_BINARY_DIR}/OSPConfig.h)
install(FILES ${CMAKE_BINARY_DIR}/OSPConfig.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK
  COMPONENT devel
)

## Build the module ##

ispc_include_directories(
  ${PROJECT_SOURCE_DIR}/ospray/include
  ${PROJECT_SOURCE_DIR}/ospray
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${RKCOMMON_INCLUDE_DIRS}
  ${EMBREE_INCLUDE_DIRS}
  ${OPENVKL_INCLUDE_DIRS}
)
add_library(ospray_module_cpu SHARED)

ispc_target_add_sources(ospray_module_cpu ${OSPRAY_ISPC_DEVICE_SOURCES})

ospray_install_library(ospray_module_cpu lib)

target_link_libraries(ospray_module_cpu
PUBLIC
  ospray
  rkcommon::rkcommon
  $<BUILD_INTERFACE:embree>
  $<BUILD_INTERFACE:openvkl::openvkl>
)

target_include_directories(ospray_module_cpu
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray/api>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray/common>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    # NOTE(jda) - the following includes are missing despite PUBLIC linking
    $<BUILD_INTERFACE:${OPENVKL_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${EMBREE_INCLUDE_DIRS}>
    #######################################################################
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(ospray_module_cpu
PUBLIC
  TILE_SIZE=${OSPRAY_TILE_SIZE}
  MAX_TILE_SIZE=${OSPRAY_MAX_STACK_TILE_SIZE}
  $<$<BOOL:${OSPRAY_DEBUG_NO_TASKING}>:OSPRAY_SERIAL_RENDERING>
)


# -------------------------------------------------------
# Install SDK headers
# -------------------------------------------------------

get_subdirectories(SUBDIRS ${CMAKE_CURRENT_LIST_DIR})

foreach(dir ${SUBDIRS})
  install(DIRECTORY ${dir}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK/modules/cpu
    COMPONENT devel
    FILES_MATCHING
    PATTERN *.h
    PATTERN *.ih
  )
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dir}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK/modules/cpu
    COMPONENT devel
    FILES_MATCHING
    PATTERN *_ispc.h
  )
endforeach()

##############################################################
# Additional interface targets
##############################################################

add_library(ospray_sdk INTERFACE)
target_link_libraries(ospray_sdk
INTERFACE
  ospray_module_cpu rkcommon::rkcommon
)
ospray_install_target(ospray_sdk devel)
