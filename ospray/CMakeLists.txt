## ======================================================================== ##
## Copyright 2009-2016 Intel Corporation                                    ##
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

IF(OSPRAY_MODULE_MPI)
  OPTION(OSPRAY_EXP_DATA_PARALLEL "Experimental data-parallel compositing mode" ON)
  OPTION(OSPRAY_PIN_ASYNC "Pin async mpi comm threads?" OFF)
  MARK_AS_ADVANCED(OSPRAY_PIN_ASYNC)
ENDIF()

OPTION(EXP_NEW_BB_VOLUME_KERNELS "Experimental new block/bricked volume layout" ON)
MARK_AS_ADVANCED(EXP_NEW_BB_VOLUME_KERNELS)

# NOTE(jda) - PThread hint used by both Embree and OSPRay
SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
SET(THREADS_PREFER_PTHREAD_FLAG TRUE) # for gcc6
FIND_PACKAGE(Threads REQUIRED)

# -------------------------------------------------------
# redistribute TBB
# -------------------------------------------------------

IF (OSPRAY_TASKING_TBB)
  IF (WIN32)
    SET(TBB_DLL_HINTS
      ${TBB_ROOT}/../redist/${TBB_ARCH}_win/tbb/${TBB_VCVER}
      ${TBB_ROOT}/../redist/${TBB_ARCH}/tbb/${TBB_VCVER}
      ${TBB_ROOT}/bin/${TBB_ARCH}/${TBB_VCVER}
    )
    FIND_FILE(TBB_DLL tbb.dll HINTS ${TBB_DLL_HINTS})
    FIND_FILE(TBB_DLL_MALLOC tbbmalloc.dll PATHS HINTS ${TBB_DLL_HINTS})
    MARK_AS_ADVANCED(TBB_DLL)
    MARK_AS_ADVANCED(TBB_DLL_MALLOC)
    IF (OSPRAY_INSTALL_DEPENDENCIES)
      INSTALL(PROGRAMS ${TBB_DLL} ${TBB_DLL_MALLOC}
              DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps) # 3rd party?
    ENDIF()
  ELSEIF (OSPRAY_ZIP_MODE)
    IF (OSPRAY_INSTALL_DEPENDENCIES)
      INSTALL(PROGRAMS ${TBB_LIBRARY} ${TBB_LIBRARY_MALLOC} ${TBB_LIBRARY_DEBUG} ${TBB_LIBRARY_MALLOC_DEBUG}
              DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib) # /intel64?
    ENDIF()
  ENDIF()
ENDIF()

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
  include/ospray/ospray.h
  include/ospray/OSPDataType.h
  include/ospray/OSPTexture.h

  math/box.ispc
  math/Distribution1D.ispc
  math/Distribution2D.ispc

  common/ospray.rc
  common/OSPCommon.ispc
  common/OSPCommon.cpp
  common/ISPC_KNL_Backend.h
  common/Managed.cpp
  common/ObjectHandle.cpp
  common/Data.cpp
  common/Model.ispc
  common/Model.cpp
  common/Material.cpp
  common/Thread.cpp
  common/Util.h

  common/tasking/parallel_for.h
  common/tasking/async.h
  common/tasking/TaskingTypeTraits.h
  common/tasking/TaskSys.cpp

  fb/FrameBuffer.ispc
  fb/FrameBuffer.cpp
  fb/LocalFB.ispc
  fb/LocalFB.cpp
  fb/PixelOp.cpp
  fb/Tile.h

  camera/Camera.cpp
  camera/Camera.ispc
  camera/PerspectiveCamera.ispc
  camera/PerspectiveCamera.cpp
  camera/OrthographicCamera.ispc
  camera/OrthographicCamera.cpp
  camera/PanoramicCamera.ispc
  camera/PanoramicCamera.cpp

  volume/BlockBrickedVolume.ispc
  volume/BlockBrickedVolume.cpp
  volume/GhostBlockBrickedVolume.ispc
  volume/GhostBlockBrickedVolume.cpp

  volume/GridAccelerator.ispc
  volume/SharedStructuredVolume.ispc
  volume/SharedStructuredVolume.cpp
  volume/StructuredVolume.ispc
  volume/StructuredVolume.cpp
  volume/Volume.ispc
  volume/Volume.cpp

  transferFunction/LinearTransferFunction.ispc
  transferFunction/LinearTransferFunction.cpp
  transferFunction/TransferFunction.ispc
  transferFunction/TransferFunction.cpp

  geometry/Geometry.ispc
  geometry/Geometry.cpp
  geometry/TriangleMesh.ispc
  geometry/TriangleMesh.cpp
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

  texture/Texture2D.cpp
  texture/Texture2D.ispc

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
  render/scivis/SciVisRenderer.ispc
  render/scivis/SciVisMaterial.ispc
  render/scivis/SciVisMaterial.ih
  render/scivis/SciVisRenderer.cpp
  render/scivis/SciVisMaterial.cpp

  render/pathtracer/PathTracer.ispc
  render/pathtracer/PathTracer.cpp
  render/pathtracer/GeometryLight.ispc
  render/pathtracer/materials/Material.ispc
  render/pathtracer/materials/OBJ.ispc
  render/pathtracer/materials/OBJ.cpp
  render/pathtracer/materials/Velvet.ispc
  render/pathtracer/materials/Velvet.cpp
  render/pathtracer/materials/Metal.ispc
  render/pathtracer/materials/Metal.cpp
  render/pathtracer/materials/ThinGlass.ispc
  render/pathtracer/materials/ThinGlass.cpp
  render/pathtracer/materials/Glass.ispc
  render/pathtracer/materials/Glass.cpp
  render/pathtracer/materials/MetallicPaint.ispc
  render/pathtracer/materials/MetallicPaint.cpp
  render/pathtracer/materials/Plastic.ispc
  render/pathtracer/materials/Plastic.cpp
  render/pathtracer/materials/Matte.ispc
  render/pathtracer/materials/Matte.cpp
  render/pathtracer/materials/Luminous.ispc
  render/pathtracer/materials/Luminous.cpp

  api/API.cpp
  api/Device.cpp
  api/LocalDevice.cpp
)

# -------------------------------------------------------
# Install API headers
# -------------------------------------------------------

OSPRAY_INSTALL_SDK_HEADERS(
  camera/Camera.h
  camera/Camera.ih
  camera/OrthographicCamera.h
  camera/OrthographicCamera.ih
  camera/PanoramicCamera.h
  camera/PanoramicCamera.ih
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
  common/Ray.h
  common/Ray.ih
  common/Texture.h
  common/Thread.h
  common/Util.h
  DESTINATION common
)

OSPRAY_INSTALL_SDK_HEADERS(
  common/tasking/async.h
  common/tasking/parallel_for.h
  common/tasking/TaskingTypeTraits.h
  common/tasking/TaskSys.h
  DESTINATION common/tasking
)

OSPRAY_INSTALL_SDK_HEADERS(
  fb/FrameBuffer.h
  fb/FrameBuffer.ih
  fb/PixelOp.h
  fb/Tile.h
  fb/Tile.ih
  DESTINATION fb
)

OSPRAY_INSTALL_SDK_HEADERS(
  geometry/Cylinders.h
  geometry/Geometry.h
  geometry/Geometry.ih
  geometry/Instance.h
  geometry/Isosurfaces.h
  geometry/Slices.h
  geometry/Spheres.h
  geometry/StreamLines.h
  geometry/TriangleMesh.h
  geometry/TriangleMesh.ih
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
  math/sampling.ih
  math/vec.ih
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
  texture/Texture2D.h
  texture/Texture2D.ih
  texture/TextureParam.ih
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
  volume/BlockBrickedVolume.h
  volume/BlockBrickedVolume.ih
  volume/GridAccelerator.ih
  volume/GhostBlockBrickedVolume.h
  volume/GhostBlockBrickedVolume.ih
  volume/SharedStructuredVolume.h
  volume/SharedStructuredVolume.ih
  volume/StructuredVolume.h
  volume/StructuredVolume.ih
  volume/Volume.h
  volume/Volume.ih
  DESTINATION volume
)

##############################################################
# build the ospray library (using flags and compilers as configured by
# CONFIGURE_OSPRAY() and/or CONFIGURE_MPI()
##############################################################

SET(OSPRAY_LIBS
  ospray_common
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
  ${EMBREE_LIBRARIES}
  ${TASKING_SYSTEM_LIBS}
)

IF (WIN32)
  LIST(APPEND OSPRAY_LIBS ws2_32)
  # export ISPC SDK symbols on Windows
  IF (OSPRAY_BUILD_ISA STREQUAL "ALL")
    LIST(APPEND OSPRAY_SOURCES ospray.def)
  ENDIF()
ENDIF()

OSPRAY_CREATE_LIBRARY(ospray ${OSPRAY_SOURCES} LINK ${OSPRAY_LIBS})

# build ospTutorial, for testing
ADD_EXECUTABLE(ospTutorial ../apps/ospTutorial.cpp)
OSPRAY_EXE_LINK_LIBRARIES(ospTutorial ospray ospray_common)
IF (OSPRAY_MODULE_MPI)
  OSPRAY_EXE_LINK_LIBRARIES(ospTutorial ospray_mpi)
ENDIF()
# C version
ADD_EXECUTABLE(ospTutorialC ../apps/ospTutorial.c)
OSPRAY_EXE_LINK_LIBRARIES(ospTutorialC ospray ospray_common)
IF (OSPRAY_MODULE_MPI)
  OSPRAY_EXE_LINK_LIBRARIES(ospTutorialC ospray_mpi)
ENDIF()

##############################################################
# Configure find_package files
##############################################################

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/ospray_cmake_config.cmake)
