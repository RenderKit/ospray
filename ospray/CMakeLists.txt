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

OPTION(EXP_NEW_BB_VOLUME_KERNELS "Experimental new block/bricked volume layout" ON)
MARK_AS_ADVANCED(EXP_NEW_BB_VOLUME_KERNELS)

# NOTE(jda) - PThread hint used by both Embree and OSPRay
SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
SET(THREADS_PREFER_PTHREAD_FLAG TRUE) # for gcc6
FIND_PACKAGE(Threads REQUIRED)

# -------------------------------------------------------
# Setup ospray include directories and source files
# -------------------------------------------------------

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/ospray/include
  ${CMAKE_SOURCE_DIR}/ospray
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${EMBREE_INCLUDE_DIRS}
)
INCLUDE_DIRECTORIES_ISPC(
  ${CMAKE_SOURCE_DIR}/ospray/include
  ${CMAKE_SOURCE_DIR}/ospray
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  ${EMBREE_INCLUDE_DIRS}
)

SET(OSPRAY_SOURCES
  api/API.cpp
  api/Device.cpp

  common/ospray.rc
  common/OSPCommon.cpp

  include/ospray/ospray.h
  include/ospray/OSPDataType.h
  include/ospray/OSPTexture.h

  include/ospray/ospray_cpp/Camera.h
  include/ospray/ospray_cpp/Data.h
  include/ospray/ospray_cpp/FrameBuffer.h
  include/ospray/ospray_cpp/Geometry.h
  include/ospray/ospray_cpp/Light.h
  include/ospray/ospray_cpp/ManagedObject.h
  include/ospray/ospray_cpp/Material.h
  include/ospray/ospray_cpp/Model.h
  include/ospray/ospray_cpp/PixelOp.h
  include/ospray/ospray_cpp/Renderer.h
  include/ospray/ospray_cpp/Texture2D.h
  include/ospray/ospray_cpp/TransferFunction.h
  include/ospray/ospray_cpp/Volume.h
)

SET(OSPRAY_ISPC_DEVICE_SOURCES
  ispc_tasksys.cpp

  api/ISPCDevice.h
  api/ISPCDevice.cpp

  common/ObjectHandle.cpp
  common/Data.cpp
  common/Managed.cpp
  common/Model.ispc
  common/Model.cpp
  common/OSPCommon.ispc
  common/Material.cpp
  common/Util.h

  fb/FrameBuffer.ispc
  fb/FrameBuffer.cpp
  fb/LocalFB.ispc
  fb/LocalFB.cpp
  fb/PixelOp.cpp
  fb/ToneMapperPixelOp.cpp
  fb/ToneMapperPixelOp.ispc
  fb/Tile.h
  fb/TileError.cpp

  camera/Camera.cpp
  camera/Camera.ispc
  camera/PerspectiveCamera.ispc
  camera/PerspectiveCamera.cpp
  camera/OrthographicCamera.ispc
  camera/OrthographicCamera.cpp
  camera/PanoramicCamera.ispc
  camera/PanoramicCamera.cpp

  geometry/Geometry.ispc
  geometry/Geometry.cpp
  geometry/TriangleMesh.ispc
  geometry/TriangleMesh.cpp
  geometry/QuadMesh.ispc
  geometry/QuadMesh.cpp
  geometry/Curves.cpp
  geometry/Curves.ispc
  geometry/StreamLines.cpp
  geometry/StreamLines.ispc
  geometry/Instance.ispc
  geometry/Instance.cpp
  geometry/Spheres.cpp
  geometry/Spheres.ispc
  geometry/Cylinders.cpp
  geometry/Cylinders.ispc
  geometry/Slices.ispc
  geometry/Slices.cpp
  geometry/Isosurfaces.ispc
  geometry/Isosurfaces.cpp

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

  math/box.ispc
  math/Distribution1D.ispc
  math/Distribution2D.ispc
  math/sobol.ih
  math/sobol.ispc
  math/spectrum.h
  math/spectrum.ih
  math/spectrum.ispc

  render/LoadBalancer.cpp
  render/Renderer.ispc
  render/Renderer.cpp
  render/util.ispc
  render/raycast/RaycastRenderer.cpp
  render/raycast/RaycastRenderer.ispc
  render/simpleAO/SimpleAO.cpp
  render/simpleAO/SimpleAO.ispc
  render/simpleAO/SimpleAOMaterial.cpp
  render/simpleAO/SimpleAOMaterial.ih
  render/simpleAO/SimpleAOMaterial.ispc
  render/scivis/lightAlpha.ispc
  render/scivis/volumeIntegration.ispc
  render/scivis/surfaceShading.ispc
  render/scivis/SciVisRenderer.ispc
  render/scivis/SciVisMaterial.ispc
  render/scivis/SciVisMaterial.ih
  render/scivis/SciVisRenderer.cpp
  render/scivis/SciVisMaterial.cpp

  render/pathtracer/PathTracer.ispc
  render/pathtracer/PathTracer.cpp
  render/pathtracer/GeometryLight.ispc
  render/pathtracer/bsdfs/MicrofacetAlbedoTables.ispc
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

  transferFunction/LinearTransferFunction.ispc
  transferFunction/LinearTransferFunction.cpp
  transferFunction/TransferFunction.ispc
  transferFunction/TransferFunction.cpp

  volume/Volume.ispc
  volume/Volume.cpp

  volume/structured/GridAccelerator.ispc
  volume/structured/StructuredVolume.ispc
  volume/structured/StructuredVolume.cpp

  volume/structured/bricked/BlockBrickedVolume.ispc
  volume/structured/bricked/BlockBrickedVolume.cpp
  volume/structured/bricked/GhostBlockBrickedVolume.ispc
  volume/structured/bricked/GhostBlockBrickedVolume.cpp

  volume/structured/shared/SharedStructuredVolume.ispc
  volume/structured/shared/SharedStructuredVolume.cpp

  volume/unstructured/MinMaxBVH2.cpp
  volume/unstructured/MinMaxBVH2.ispc
  volume/unstructured/UnstructuredVolume.cpp
  volume/unstructured/UnstructuredVolume.ispc

  volume/amr/AMRAccel.cpp
  volume/amr/AMRData.cpp
  volume/amr/AMRVolume.ispc
  volume/amr/AMRVolume.cpp
  volume/amr/CellRef.ispc
  volume/amr/DualCell.ispc
  volume/amr/method_current.ispc
  volume/amr/method_finest.ispc
  volume/amr/method_octant.ispc
)


# -------------------------------------------------------
# Install API headers
# -------------------------------------------------------

OSPRAY_INSTALL_SDK_HEADERS(
  api/Device.h
  api/ISPCDevice.h
  api/objectFactory.h
  DESTINATION api
)

OSPRAY_INSTALL_SDK_HEADERS(
  camera/Camera.h
  camera/Camera.ih
  camera/OrthographicCamera.h
  camera/OrthographicCamera.ih
  camera/PanoramicCamera.h
  camera/PerspectiveCamera.h
  camera/PerspectiveCamera.ih
  DESTINATION camera
)

OSPRAY_INSTALL_SDK_HEADERS(
  common/Data.h
  common/DifferentialGeometry.ih
  common/Library.h
  common/Managed.h
  common/Material.h
  common/Material.ih
  common/Model.h
  common/Model.ih
  common/ObjectHandle.h
  common/OSPCommon.h
  common/OSPCommon.ih
  common/Ray.ih
  common/Texture.h
  common/Util.h
  DESTINATION common
)

OSPRAY_INSTALL_SDK_HEADERS(
  fb/FrameBuffer.h
  fb/FrameBuffer.ih
  fb/LocalFB.h
  fb/LocalFB.ih
  fb/PixelOp.h
  fb/Tile.h
  fb/Tile.ih
  fb/TileError.h
  DESTINATION fb
)

OSPRAY_INSTALL_SDK_HEADERS(
  geometry/Curves.h
  geometry/Cylinders.h
  geometry/Geometry.h
  geometry/Geometry.ih
  geometry/Instance.h
  geometry/Instance.ih
  geometry/Isosurfaces.h
  geometry/Slices.h
  geometry/Spheres.h
  geometry/StreamLines.h
  geometry/TriangleMesh.h
  geometry/TriangleMesh.ih
  geometry/QuadMesh.h
  geometry/QuadMesh.ih
  DESTINATION geometry
)

OSPRAY_INSTALL_SDK_HEADERS(
  lights/AmbientLight.h
  lights/DirectionalLight.h
  lights/HDRILight.h
  lights/Light.h
  lights/Light.ih
  lights/PointLight.h
  lights/QuadLight.h
  lights/SpotLight.h
  DESTINATION lights
)

OSPRAY_INSTALL_SDK_HEADERS(
  #FIXME: check if all of these are used...
  math/AffineSpace.ih
  math/box.ih
  math/Distribution1D.ih
  math/Distribution2D.ih
  math/LinearSpace.ih
  math/math.ih
  math/random.ih
  math/sobol.ih
  math/sampling.ih
  math/vec.ih
  math/spectrum.h
  math/spectrum.ih
  DESTINATION math
)

OSPRAY_INSTALL_SDK_HEADERS(
  render/LoadBalancer.h
  render/Renderer.h
  render/Renderer.ih
  render/util.h
  render/util.ih
  DESTINATION render
)

#TODO: all the specific renderer headers...

OSPRAY_INSTALL_SDK_HEADERS(
  texture/Texture.h
  texture/Texture2D.h
  texture/Texture2D.ih
  texture/TextureParam.ih
  texture/TextureVolume.h
  DESTINATION texture
)

OSPRAY_INSTALL_SDK_HEADERS(
  transferFunction/LinearTransferFunction.h
  transferFunction/LinearTransferFunction.ih
  transferFunction/TransferFunction.h
  transferFunction/TransferFunction.ih
  DESTINATION transferFunction
)

OSPRAY_INSTALL_SDK_HEADERS(
  volume/Volume.h
  volume/Volume.ih
  DESTINATION volume
)

OSPRAY_INSTALL_SDK_HEADERS(
  volume/amr/AMR.ih
  volume/amr/AMRAccel.h
  volume/amr/AMRData.h
  volume/amr/AMRVolume.h
  volume/amr/AMRVolume.ih
  volume/amr/CellRef.ih
  volume/amr/DualCell.ih
  volume/amr/FindStack.ih
  volume/amr/KDTree.ih
  volume/amr/range.ih
  DESTINATION volume/amr
)

OSPRAY_INSTALL_SDK_HEADERS(
  volume/structured/GridAccelerator.ih
  volume/structured/StructuredVolume.h
  volume/structured/StructuredVolume.ih
  DESTINATION volume/structured
)

OSPRAY_INSTALL_SDK_HEADERS(
  volume/structured/bricked/BlockBrickedVolume.h
  volume/structured/bricked/BlockBrickedVolume.ih
  volume/structured/bricked/GhostBlockBrickedVolume.h
  volume/structured/bricked/GhostBlockBrickedVolume.ih
  DESTINATION volume/structured/bricked
)

OSPRAY_INSTALL_SDK_HEADERS(
  volume/structured/shared/SharedStructuredVolume.h
  volume/structured/shared/SharedStructuredVolume.ih
  DESTINATION volume/structured/shared
)

OSPRAY_INSTALL_SDK_HEADERS(
  volume/unstructured/MinMaxBVH2.h
  volume/unstructured/MinMaxBVH2.ih
  volume/unstructured/UnstructuredVolume.h
  volume/unstructured/UnstructuredVolume.ih
  DESTINATION volume/unstructured
)

OSPRAY_INSTALL_SDK_HEADERS(
  include/ospray/ospray_cpp/Camera.h
  include/ospray/ospray_cpp/Data.h
  include/ospray/ospray_cpp/FrameBuffer.h
  include/ospray/ospray_cpp/Geometry.h
  include/ospray/ospray_cpp/Light.h
  include/ospray/ospray_cpp/ManagedObject.h
  include/ospray/ospray_cpp/Material.h
  include/ospray/ospray_cpp/Model.h
  include/ospray/ospray_cpp/PixelOp.h
  include/ospray/ospray_cpp/Renderer.h
  include/ospray/ospray_cpp/Texture2D.h
  include/ospray/ospray_cpp/TransferFunction.h
  include/ospray/ospray_cpp/Volume.h
  DESTINATION ../ospray_cpp
)

##############################################################
# build the ospray library (using flags and compilers as configured by
# CONFIGURE_OSPRAY() and/or CONFIGURE_MPI()
##############################################################


SET(OSPRAY_LIBS
  ospray_common
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)

IF (WIN32)
  LIST(APPEND OSPRAY_LIBS ws2_32)

  # -------------------------------------------------------
  # export ISPC SDK symbols on Windows ##
  # -------------------------------------------------------

  FILE(READ def_header.txt OSPRAY_DEF)
  FILE(READ ispc_symbols.txt OSPRAY_ISPC_SYMBOLS_IN)

  FOREACH(isa ${OSPRAY_ISPC_TARGET_LIST})
    STRING(REPLACE "-i32x16" "" isa ${isa}) # strip avx512(knl|skx)-i32x16
    # add isa suffix
    STRING(REPLACE "," ${isa} OSPRAY_ISPC_SYMBOLS ${OSPRAY_ISPC_SYMBOLS_IN})
    STRING(APPEND OSPRAY_DEF ${OSPRAY_ISPC_SYMBOLS})
  ENDFOREACH()

  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ospray_module_ispc.def.in "${OSPRAY_DEF}")

  # changes ospray.def only if content changed, avoids unnecessary re-linking
  CONFIGURE_FILE(${CMAKE_CURRENT_BINARY_DIR}/ospray_module_ispc.def.in ospray_module_ispc.def COPYONLY)

  LIST(APPEND OSPRAY_ISPC_DEVICE_SOURCES ospray_module_ispc.def)
ENDIF()

OSPRAY_CREATE_LIBRARY(ospray
  ${OSPRAY_SOURCES}
LINK
  ${OSPRAY_LIBS}
COMPONENT
  lib
)

OPTION(OSPRAY_BUILD_ISPC_DEVICE "Build ISPC Device" ON)
MARK_AS_ADVANCED(OSPRAY_BUILD_ISPC_DEVICE)

IF(OSPRAY_BUILD_ISPC_DEVICE)
  OSPRAY_CREATE_LIBRARY(ospray_module_ispc
    ${OSPRAY_ISPC_DEVICE_SOURCES}
  LINK
    ${OSPRAY_LIBS}
    ospray
    ${EMBREE_LIBRARIES}
  COMPONENT
    lib
  )
ENDIF()

OPTION(OSPRAY_ENABLE_TUTORIAL
  "Build the tutorial executable" ON)
MARK_AS_ADVANCED(OSPRAY_ENABLE_TUTORIAL)
IF(OSPRAY_ENABLE_TUTORIAL)
  # build ospTutorial, for demonstration and API testing
  ADD_EXECUTABLE(ospTutorial ../apps/ospTutorial.c)
  TARGET_LINK_LIBRARIES(ospTutorial ospray)
  # C++ version
  ADD_EXECUTABLE(ospTutorialCpp ../apps/ospTutorial.cpp)
  TARGET_LINK_LIBRARIES(ospTutorialCpp ospray)
ENDIF()

##############################################################
# Configure find_package files
##############################################################

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/ospray_cmake_config.cmake)
