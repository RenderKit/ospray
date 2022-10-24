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

cmake_dependent_option(
  OSPRAY_MODULE_GPU
  "Build the GPU module"
  ON
  "OSPRAY_ENABLE_MODULES;EMBREE_SYCL_SUPPORT"
  OFF
)
mark_as_advanced(OSPRAY_MODULE_GPU)

if ((NOT OSPRAY_MODULE_CPU) AND (NOT OSPRAY_MODULE_GPU))
  return()
endif()

# -------------------------------------------------------
# Setup module include directories and source files
# -------------------------------------------------------

set(OSPRAY_CPP_SOURCES
  ${OSPRAY_RESOURCE}

  ModuleInit.cpp
  ispc_tasksys.cpp

  ISPCDevice.h
  ISPCDevice.cpp

  common/Data.cpp
  common/Future.cpp
  common/Group.cpp
  common/Instance.cpp
  common/MotionTransform.cpp
  common/Managed.cpp
  common/World.cpp

  fb/FrameBuffer.cpp
  fb/FrameBufferView.cpp
  fb/LocalFB.cpp
  fb/SparseFB.cpp
  fb/ImageOp.cpp
  fb/TaskError.cpp
  fb/registration.cpp

  fb/frame_ops/Blur.cpp
  fb/frame_ops/Debug.cpp
  fb/frame_ops/Depth.cpp
  fb/frame_ops/SSAO.cpp

  fb/pixel_ops/ToneMapper.cpp

  pf/PixelFilter.cpp

  camera/Camera.cpp
  camera/PerspectiveCamera.cpp
  camera/OrthographicCamera.cpp
  camera/PanoramicCamera.cpp
  camera/registration.cpp

  geometry/Geometry.cpp
  geometry/GeometricModel.cpp
  geometry/Boxes.cpp
  geometry/Curves.cpp
  geometry/Spheres.cpp
  geometry/Planes.cpp
  geometry/Mesh.cpp
  geometry/registration.cpp

  lights/Light.cpp
  lights/AmbientLight.cpp
  lights/DirectionalLight.cpp
  lights/PointLight.cpp
  lights/IntensityDistribution.cpp
  lights/SpotLight.cpp
  lights/QuadLight.cpp
  lights/CylinderLight.cpp
  lights/HDRILight.cpp
  lights/SunSkyLight.cpp
  lights/sky_model/sky_model.cpp
  lights/registration.cpp

  math/spectrum.h
  math/halton.cpp
  math/sobol.cpp
  math/MathConstants.cpp
  math/Distribution2D.cpp

  render/LoadBalancer.cpp
  render/Material.cpp
  render/Renderer.cpp
  render/registration.cpp

  render/debug/DebugRenderer.cpp

  render/ao/AORenderer.cpp

  render/scivis/SciVis.cpp
  render/scivis/SciVisData.cpp

  render/pathtracer/PathTracer.cpp
  render/pathtracer/PathTracerData.cpp

  render/materials/OBJ.cpp
  render/materials/Principled.cpp
  render/materials/CarPaint.cpp
  render/materials/Velvet.cpp
  render/materials/Metal.cpp
  render/materials/Alloy.cpp
  render/materials/ThinGlass.cpp
  render/materials/Glass.cpp
  render/materials/MetallicPaint.cpp
  render/materials/Plastic.cpp
  render/materials/Luminous.cpp
  render/materials/Mix.cpp

  render/bsdfs/MicrofacetAlbedoTables.cpp

  texture/Texture.cpp
  texture/Texture2D.cpp
  texture/registration.cpp
)

set(OSPRAY_ISPC_SOURCES
  ISPCDevice.ispc

  common/OSPCommon.ispc
  common/World.ispc

  fb/FrameBuffer.ispc
  fb/FrameBufferType.ispc
  fb/LocalFB.ispc
  fb/SparseFB.ispc

  fb/frame_ops/SSAO.ispc

  fb/pixel_ops/ToneMapper.ispc

  pf/PixelFilterType.ispc
  pf/LUTPixelFilter.ispc

  camera/CameraType.ispc
  camera/PerspectiveCamera.ispc
  camera/OrthographicCamera.ispc
  camera/PanoramicCamera.ispc

  geometry/GeometryType.ispc
  geometry/GeometricModel.ispc
  geometry/Boxes.ispc
  geometry/Curves.ispc
  geometry/Spheres.ispc
  geometry/Planes.ispc
  geometry/Mesh.ispc

  lights/LightType.ispc
  lights/Light.ispc
  lights/AmbientLight.ispc
  lights/DirectionalLight.ispc
  lights/PointLight.ispc
  lights/SpotLight.ispc
  lights/QuadLight.ispc
  lights/CylinderLight.ispc
  lights/HDRILight.ispc

  math/Distribution1D.ispc
  math/Distribution2D.ispc
  math/spectrum.ispc

  render/Material.ispc
  render/MaterialType.ispc
  render/Renderer.ispc
  render/RendererType.ispc
  render/util.ispc

  render/debug/DebugRenderer.ispc

  render/ao/AORenderer.ispc
  render/ao/surfaces.ispc

  render/scivis/SciVis.ispc
  render/scivis/surfaces.ispc
  render/scivis/lightAlpha.ispc

  render/pathtracer/PathTracer.ispc
  render/pathtracer/PathSampler.ispc
  render/pathtracer/GeometryLight.ispc
  render/pathtracer/VirtualLight.ispc
  render/pathtracer/TransparentShadow.ispc
  render/pathtracer/ShadowCatcher.ispc
  render/pathtracer/NextEventEstimation.ispc

  render/bsdfs/BSDF.ispc
  render/bsdfs/MicrofacetAlbedoTables.ispc
  render/materials/OBJ.ispc
  render/materials/Principled.ispc
  render/materials/CarPaint.ispc
  render/materials/Velvet.ispc
  render/materials/Metal.ispc
  render/materials/Alloy.ispc
  render/materials/ThinGlass.ispc
  render/materials/Glass.ispc
  render/materials/MetallicPaint.ispc
  render/materials/Plastic.ispc
  render/materials/Luminous.ispc
  render/materials/Mix.ispc

  texture/Texture2D.ispc
  texture/TextureType.ispc
)

if (OSPRAY_ENABLE_VOLUMES)
  set(OSPRAY_VOLUMES_CPP_SOURCES
    geometry/Isosurfaces.cpp

    texture/TextureVolume.cpp

    volume/Volume.cpp
    volume/VolumetricModel.cpp

    volume/transferFunction/LinearTransferFunction.cpp
    volume/transferFunction/TransferFunction.cpp
    volume/transferFunction/registration.cpp
  )

  set(OSPRAY_VOLUMES_ISPC_SOURCES
    geometry/Isosurfaces.ispc

    render/ao/volumes.ispc
    render/scivis/volumes.ispc
    render/pathtracer/volumes/VolumeSampler.ispc

    texture/TextureVolume.ispc

    volume/Volume.ispc
    volume/transferFunction/LinearTransferFunction.ispc
  )
endif()

##############################################################
# Build ISPC device
##############################################################


## Windows-specifc ISPC symbol exports ##

if (WIN32)
  file(READ def_header.txt OSPRAY_DEF)
  file(READ ispc_symbols.txt OSPRAY_ISPC_SYMBOLS_IN)

  list(LENGTH OSPRAY_ISPC_TARGET_LIST NUM_TARGETS)
  foreach(isa ${OSPRAY_ISPC_TARGET_LIST})
    string(REPLACE "-i32x16" "" isa ${isa}) # strip avx512(knl|skx)-i32x16
    if (NUM_TARGETS EQUAL 1)
      set(isa "") # for single target no suffix
      string(REPLACE "delete_uniform_," "delete_uniform," OSPRAY_ISPC_SYMBOLS_IN ${OSPRAY_ISPC_SYMBOLS_IN})
    endif()
    # add isa suffix
    string(REPLACE "," "${isa}" OSPRAY_ISPC_SYMBOLS ${OSPRAY_ISPC_SYMBOLS_IN})
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

configure_file(common/OSPConfig.h.in ${PROJECT_BINARY_DIR}/OSPConfig.h)
install(FILES ${CMAKE_BINARY_DIR}/OSPConfig.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK
  COMPONENT devel
)

## Build the module ##

if (OSPRAY_MODULE_CPU)
  include_directories_ispc(
    ${PROJECT_SOURCE_DIR}/ospray/include
    ${PROJECT_SOURCE_DIR}/ospray
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
    ${RKCOMMON_INCLUDE_DIRS}
    ${EMBREE_INCLUDE_DIRS}
  )

  add_definitions_ispc(
    -DOSPRAY_BEGIN_ISPC_NAMESPACE=
    -DOSPRAY_END_ISPC_NAMESPACE=
    -DSYCL_EXTERNAL=
    -D__noinline=
  )

  if (OSPRAY_ENABLE_VOLUMES)
    include_directories_ispc(
      ${OPENVKL_INCLUDE_DIRS}
    )

    add_definitions_ispc(
      -DOSPRAY_ENABLE_VOLUMES
    )
  endif()

  add_library(ospray_module_cpu SHARED)

  ispc_target_add_sources(ospray_module_cpu
      ${OSPRAY_CPP_SOURCES}
      ${OSPRAY_VOLUMES_CPP_SOURCES}
      ${OSPRAY_ISPC_SOURCES}
      ${OSPRAY_VOLUMES_ISPC_SOURCES}
      geometry/Subdivision.cpp
      geometry/Subdivision.ispc)

  ospray_install_library(ospray_module_cpu lib)

  target_link_libraries(ospray_module_cpu
  PUBLIC
    ospray
    rkcommon::rkcommon
    $<BUILD_INTERFACE:ispcrt::ispcrt>
    $<BUILD_INTERFACE:embree>
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
      $<BUILD_INTERFACE:${ISPCRT_INCLUDE_DIR}>
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
    SYCL_EXTERNAL=
    __noinline=
  )

  if (OSPRAY_ENABLE_VOLUMES)
    target_link_libraries(ospray_module_cpu
    PUBLIC
      $<BUILD_INTERFACE:openvkl::openvkl>
    )

    target_include_directories(ospray_module_cpu
    PUBLIC
      $<BUILD_INTERFACE:${OPENVKL_INCLUDE_DIRS}>
    )

    target_compile_definitions(ospray_module_cpu
    PUBLIC
      -DOSPRAY_ENABLE_VOLUMES
    )
  endif()
endif()

if (OSPRAY_MODULE_GPU)
  include(${ISPCRT_DIR}/interop.cmake)
  if (NOT "${CMAKE_CXX_COMPILER}" STREQUAL "${DPCPP_COMPILER}")
    message(FATAL_ERROR "CMAKE_CXX_COMPILER should be set to '${DPCPP_COMPILER}' for SYCL")
  endif()

  ospray_configure_dpcpp_target()

  # Have CMake's build infrastructure treat the ISPC files as C++ for SYCL
  set_source_files_properties(${OSPRAY_ISPC_SOURCES} PROPERTIES
      LANGUAGE CXX)

  add_library(ospray_module_gpu SHARED
      ${OSPRAY_CPP_SOURCES} ${OSPRAY_ISPC_SOURCES})

  set_target_properties(ospray_module_gpu PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

  target_link_libraries(ospray_module_gpu
  PUBLIC
    ospray
    rkcommon::rkcommon
    $<BUILD_INTERFACE:ispcrt::ispcrt>
    $<BUILD_INTERFACE:embree>
    # TODO: needs OpenVKL GPU port
    #$<BUILD_INTERFACE:openvkl::openvkl>
  )

  target_include_directories(ospray_module_gpu
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray/api>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray/common>
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ospray>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
      # NOTE(jda) - the following includes are missing despite PUBLIC linking
      $<BUILD_INTERFACE:${ISPCRT_INCLUDE_DIR}>
      # TODO: needs OpenVKL GPU port
      #$<BUILD_INTERFACE:${OPENVKL_INCLUDE_DIRS}>
      $<BUILD_INTERFACE:${EMBREE_INCLUDE_DIRS}>
      #######################################################################
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ospray/SDK>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_compile_definitions(ospray_module_gpu
  PUBLIC
    TILE_SIZE=${OSPRAY_TILE_SIZE}
    MAX_TILE_SIZE=${OSPRAY_MAX_STACK_TILE_SIZE}
    RKCOMMON_SYCL
    RKCOMMON_NO_SIMD
  )

  target_compile_options(ospray_module_gpu
  PRIVATE
    -x c++
  )

  target_compile_options(ospray_module_gpu PRIVATE
    -fsycl
    ${OSPRAY_CXX_FLAGS_SYCL}
    -fsycl-targets=${OSPRAY_SYCL_TARGET}
  )

  #include(ProcessorCount)
  #ProcessorCount(PROCESSOR_COUNT)
  #if (NOT PROCESSOR_COUNT EQUAL 0)
  #  set(SYCL_MAX_LINK_JOBS ${PROCESSOR_COUNT})
  #else()
  #  set(SYCL_MAX_LINK_JOBS 4)
  #endif()

  if (OSPRAY_SYCL_AOT_DEVICES STREQUAL "none")
    target_link_options(ospray_module_gpu PRIVATE
      -fsycl
      #-fsycl-device-code-split=per_kernel
      #-fsycl-max-parallel-link-jobs=${SYCL_MAX_LINK_JOBS}
      -fsycl-targets=${OSPRAY_SYCL_TARGET}
      -Xsycl-target-backend=${OSPRAY_SYCL_TARGET} "${OSPRAY_OCL_OPTIONS_STR} -options \"${OSPRAY_OCL_OTHER_OPTIONS_STR} -igc_opts='${OSPRAY_IGC_OPTIONS_STR}'\""
    )
  else()
    target_link_options(ospray_module_gpu PRIVATE
      -fsycl
      #-fsycl-device-code-split=per_kernel
      #-fsycl-max-parallel-link-jobs=${SYCL_MAX_LINK_JOBS}
      -fsycl-targets=${OSPRAY_SYCL_TARGET}
      -Xsycl-target-backend=${OSPRAY_SYCL_TARGET} "-device ${OSPRAY_SYCL_AOT_DEVICES} -revision_id ${OSPRAY_SYCL_AOT_DEVICE_REVISION} ${OSPRAY_OCL_OPTIONS_STR} -options \"${OSPRAY_OCL_OTHER_OPTIONS_STR} -igc_opts='${OSPRAY_IGC_OPTIONS_STR}'\""
    )
  endif()


  target_compile_definitions(ospray_module_gpu
  PUBLIC
    OSPRAY_TARGET_SYCL
    __SYCL_USE_NON_VARIADIC_SPIRV_OCL_PRINTF__
    OSPRAY_BEGIN_ISPC_NAMESPACE=namespace\ ispc{
    OSPRAY_END_ISPC_NAMESPACE=}
    uniform=
    varying=
    unmasked=
    export=
    make_vec2i=vec2i
    make_vec3i=vec3i
    make_vec4i=vec4i
    make_vec2ui=vec2ui
    make_vec3ui=vec3ui
    make_vec4ui=vec4ui
    make_vec2f=vec2f
    make_vec3f=vec3f
    make_vec4f=vec4f
    make_box1f=box1f
    make_box2f=box2f
    make_box3f=box3f
    make_box3fa=box3fa
    make_box4f=box4f
    make_LinearSpace3f=LinearSpace3f
    make_LinearSpace3f_varying_identity=LinearSpace3f
    rtcIntersectV=rtcIntersect1
    rtcIntersectExV=rtcIntersectEx1
    rtcOccludedV=rtcOccluded1
    rtcOccludedExV=rtcOccludedEx1
    rtcInterpolateV1=rtcInterpolate1
    programCount=1
    programIndex=0
    signbits=sycl::signbit
  )

  ospray_install_library(ospray_module_gpu lib)
endif()

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
  ospray
  rkcommon::rkcommon
)
ospray_install_target(ospray_sdk devel)
