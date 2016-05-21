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

CONFIGURE_OSPRAY()

IF(OSPRAY_BUILD_MPI_DEVICE)
  OPTION(OSPRAY_EXP_DATA_PARALLEL "Experimental data-parallel compositing mode")
  OPTION(OSPRAY_PIN_ASYNC "Pin async mpi comm threads?" OFF)
  MARK_AS_ADVANCED(OSPRAY_PIN_ASYNC)
ENDIF()

OPTION(EXP_NEW_BB_VOLUME_KERNELS "Experimental new block/bricked volume layout" ON)
MARK_AS_ADVANCED(EXP_NEW_BB_VOLUME_KERNELS)

# NOTE(jda) - PThread hint used by both Embree and OSPRay
SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
FIND_PACKAGE(Threads REQUIRED)

# -------------------------------------------------------
# Find or build Embree
# -------------------------------------------------------

SET(REQUIRED_MINIMUM_EMBREE 2.7.1)

IF(OSPRAY_USE_EXTERNAL_EMBREE)
  # Clear out embree directories if they were previously populated by an
  # internal build
  IF(NOT ${LAST_CONFIG_USED_EXTERNAL_EMBREE})
    UNSET(EMBREE_INCLUDE_DIRS)
    UNSET(EMBREE_LIBRARIES)
    UNSET(EMBREE_LIBRARY)
    UNSET(EMBREE_LIBRARY_XEONPHI)
  ENDIF()

  # Find existing Embree on the machine
  FIND_PACKAGE(embree ${REQUIRED_MINIMUM_EMBREE} REQUIRED)
  SET(LAST_CONFIG_USED_EXTERNAL_EMBREE ON CACHE INTERNAL "" FORCE)

  # NOTE(jda) - EMBREE_LIBRARIES is not defined until at lest v2.10.0, for now
  #             create a "faked" EMBREE_LIBRARIES until we set our min version
  #             to >= 2.10.0 of Embree
  IF(NOT DEFINED EMBREE_LIBRARIES)
    SET(EMBREE_LIBRARIES ${EMBREE_LIBRARY})
  ELSE()
    # NOTE(jda) - We are using the updated Embree find_package() config, check
    #             if we need to add TBB to EMBREE_LIBRARIES
    IF(${EMBREE_TASKING_TBB} AND NOT ${EMBREE_USE_PACKAGED_TBB})
      OSPRAY_WARN_ONCE(EMBREE_FORCE_TBB
        "WARNING: You *MUST* have TBB installed based on the Embree we found!")
      FIND_PACKAGE(TBB REQUIRED)
      SET(EMBREE_LIBRARIES ${EMBREE_LIBRARIES} ${TBB_LIBRARIES})
    ENDIF()
  ENDIF()
ELSE()
  # Clear out embree directories if they were previously populated by an
  # external find_package() call
  IF(${LAST_CONFIG_USED_EXTERNAL_EMBREE})
    UNSET(EMBREE_INCLUDE_DIRS)
    UNSET(EMBREE_LIBRARIES)
    UNSET(EMBREE_LIBRARY)
    UNSET(EMBREE_LIBRARY_XEONPHI)
  ENDIF()
  # Build Embree included in the OSPRay tree

  # NOTE(jda) - Embree assumes that OSPRAY_TASKING_TBB will be defined correctly
  #             in CONFIGURE_OSPRAY().CONFIGURE_TASKING_SYSTEM()
  # NOTE(jda) - Only do the Embree include once (Xeon), it will build both
  #             Xeon and MIC code if both are enabled.
  IF (NOT THIS_IS_MIC)
    INCLUDE(../cmake/build_embree.cmake)
  ENDIF()
  SET(EMBREE_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/ospray/embree-v2.7.1/include)
  SET(EMBREE_LIBRARY embree)
  SET(EMBREE_LIBRARIES ${EMBREE_LIBRARY})
  SET(EMBREE_LIBRARY_XEONPHI embree_xeonphi)
  SET(LAST_CONFIG_USED_EXTERNAL_EMBREE OFF CACHE INTERNAL "" FORCE)
ENDIF()

INCLUDE_DIRECTORIES(${EMBREE_INCLUDE_DIRS})
INCLUDE_DIRECTORIES_ISPC(${EMBREE_INCLUDE_DIRS})

SET(EMBREE_INCLUDE_DIRS    ${EMBREE_INCLUDE_DIRS}    PARENT_SCOPE)
SET(EMBREE_LIBRARIES       ${EMBREE_LIBRARIES}       PARENT_SCOPE)
SET(EMBREE_LIBRARY         ${EMBREE_LIBRARY}         PARENT_SCOPE)
SET(EMBREE_LIBRARY_XEONPHI ${EMBREE_LIBRARY_XEONHPI} PARENT_SCOPE)

# -------------------------------------------------------
# Setup ospray source files
# -------------------------------------------------------

SET(OSPRAY_SOURCES
  include/ospray/ospray.h
  include/ospray/OSPDataType.h
  include/ospray/OSPTexture.h

  math/box.ispc
  math/Distribution2D.ispc

  common/ospray.rc
  common/OSPCommon.ispc
  common/OSPCommon.cpp
  common/Core.cpp
  common/ISPC_KNC_Backend.h
  common/ISPC_KNL_Backend.h
  common/Managed.cpp
  common/ObjectHandle.cpp
  common/Data.cpp
  common/Model.ispc
  common/Model.cpp
  common/Material.cpp
  common/Thread.cpp

  common/tasking/parallel_for.h
  common/tasking/async.h
  common/tasking/TaskingTypeTraits.h
  common/tasking/TaskSys.cpp

  fb/FrameBuffer.ispc
  fb/FrameBuffer.cpp
  fb/LocalFB.ispc
  fb/LocalFB.cpp
  fb/PixelOp.cpp

  camera/Camera.cpp
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
  volume/DataDistributedBlockedVolume.ispc
  volume/DataDistributedBlockedVolume.cpp

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
  render/volume/RaycastVolumeMaterial.cpp
  render/volume/RaycastVolumeMaterial.ih
  render/volume/RaycastVolumeMaterial.ispc
  render/volume/RaycastVolumeRenderer.cpp
  render/volume/RaycastVolumeRenderer.ispc

  render/pathtracer/PathTracer.ispc
  render/pathtracer/PathTracer.cpp
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

  api/API.cpp
  api/Device.cpp
  api/LocalDevice.cpp
  )

# -------------------------------------------------------
# MPI components
# -------------------------------------------------------
IF (OSPRAY_MPI)
  # if mpi mode is enabled, we have to configure the right mpi
  # compiler etc.
  CONFIGURE_MPI()

  SET(OSPRAY_SOURCES ${OSPRAY_SOURCES}
    mpi/buffers.h
    mpi/command.h

    mpi/MPIDevice.cpp
    mpi/MPICommon.cpp
    mpi/MPILoadBalancer.cpp
    mpi/worker.cpp

    mpi/async/Messaging.cpp
    mpi/async/SimpleSendRecvMessaging.cpp
    mpi/async/MultiIsendIrecvMessaging.cpp
    mpi/async/BatchedIsendIrecvMessaging.cpp
    mpi/async/CommLayer.cpp

    mpi/DistributedFrameBuffer.cpp
    mpi/DistributedFrameBuffer.ispc

    fb/DisplayWall.cpp
    )
ENDIF()

# -------------------------------------------------------
# DisplayCluster components
# -------------------------------------------------------
INCLUDE(${PROJECT_SOURCE_DIR}/cmake/displaycluster.cmake)

IF (OSPRAY_DISPLAYCLUSTER)
  CONFIGURE_DISPLAYCLUSTER()
ENDIF()

# -------------------------------------------------------
# COI components
# -------------------------------------------------------
IF (OSPRAY_BUILD_COI_DEVICE)
  FIND_LIBRARY(LIBCOI_DEVICE libcoi_device.so
    PATHS
    /opt/mpss/*/sysroots/k1om-mpss-linux/usr/lib64
    /opt/intel/mic/coi/device-linux-release/lib
    )
  FIND_LIBRARY(LIBCOI_HOST libcoi_host.so
    PATHS
    /opt/mpss/*/sysroots/k1om-mpss-linux/usr/lib64
    /opt/intel/mic/coi/host-linux-release/lib
    )
  MARK_AS_ADVANCED(LIBCOI_DEVICE)
  MARK_AS_ADVANCED(LIBCOI_HOST)
  SET(OSPRAY_MIC_COI ON PARENT_SCOPE)
  # NOTE(jda) - hardcoded paths (!)...use a find_package() here?
  INCLUDE_DIRECTORIES(/opt/intel/mic/coi/include)
  INCLUDE_DIRECTORIES(/usr/include/intel-coi)
  IF (NOT THIS_IS_MIC)
    SET(OSPRAY_COI_COMPONENTS api/COIDeviceHost.cpp)
  ENDIF()
  SET(OSPRAY_SOURCES ${OSPRAY_SOURCES} ${OSPRAY_COI_COMPONENTS})
ENDIF()

##############################################################
# build the ospray library (using flags and compilers as configured by
# CONFIGURE_OSPRAY() and/or CONFIGURE_MPI()
##############################################################

IF (THIS_IS_MIC)
  ADD_DEFINITIONS(-DOSPRAY_TARGET_MIC)
ENDIF()

OSPRAY_ADD_LIBRARY(ospray SHARED ${OSPRAY_SOURCES})

OSPRAY_LIBRARY_LINK_LIBRARIES(ospray
  ospray_common
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)

IF (THIS_IS_MIC)
  OSPRAY_LIBRARY_LINK_LIBRARIES(ospray
    ${EMBREE_LIBRARY_XEONPHI}
    ${TASKING_SYSTEM_LIBS_MIC}
  )
ELSE()
  OSPRAY_LIBRARY_LINK_LIBRARIES(ospray
    ${EMBREE_LIBRARIES}
    ${TASKING_SYSTEM_LIBS}
  )
ENDIF()

IF (OSPRAY_DISPLAYCLUSTER)
  OSPRAY_LIBRARY_LINK_LIBRARIES(ospray ${DISPLAYCLUSTER_LIBRARIES})
ENDIF()

IF (WIN32)
  OSPRAY_LIBRARY_LINK_LIBRARIES(ospray ws2_32)
ENDIF()

OSPRAY_SET_LIBRARY_VERSION(ospray)
OSPRAY_INSTALL_LIBRARY(ospray)

IF(NOT THIS_IS_MIC)
  # build ospTutorial, for testing
  ADD_EXECUTABLE(ospTutorial ../apps/ospTutorial.cpp)
  OSPRAY_EXE_LINK_LIBRARIES(ospTutorial ospray ospray_common)
  # C version
  ADD_EXECUTABLE(ospTutorialC ../apps/ospTutorial.c)
  OSPRAY_EXE_LINK_LIBRARIES(ospTutorialC ospray ospray_common)
ENDIF()


##############################################################
# MPI DEVICE - mpi worker
##############################################################
IF (OSPRAY_MPI)
  IF (THIS_IS_MIC)
    OSPRAY_LIBRARY_LINK_LIBRARIES(ospray
      ${MPI_LIBRARY_MIC}
    )
  ELSE()
    OSPRAY_LIBRARY_LINK_LIBRARIES(ospray
      ${MPI_CXX_LIBRARIES}
    )
  ENDIF()

  OSPRAY_ADD_EXECUTABLE(ospray_mpi_worker mpi/MPIWorker.cpp)
  OSPRAY_EXE_LINK_LIBRARIES(ospray_mpi_worker ospray ospray_common)
  OSPRAY_INSTALL_EXE(ospray_mpi_worker)
ENDIF()


##############################################################
# COI DEVICE
##############################################################
IF (OSPRAY_BUILD_COI_DEVICE)
  IF (THIS_IS_MIC)
    # ------------------------------------------------------------
    # dev-side of COI device: lib dev-side libospray to coi dev libs,...
    # ------------------------------------------------------------
    OSPRAY_LIBRARY_LINK_LIBRARIES(ospray
      ${LIBCOI_DEVICE}
    )
    # ... and add the coi worker executable
    OSPRAY_ADD_EXECUTABLE(ospray_coi_worker api/COIDeviceWorker.cpp)
    OSPRAY_EXE_LINK_LIBRARIES(ospray_coi_worker
      ospray
      ospray_common
      ${LIBCOI_DEVICE}
    )
    # ------------------------------------------------------------
    OSPRAY_INSTALL_EXE(ospray_coi_worker)
  ELSE()
    # ------------------------------------------------------------
    # host-side of COI device: just link libospray to coi host libs
    # ------------------------------------------------------------
    OSPRAY_LIBRARY_LINK_LIBRARIES(ospray
      ${LIBCOI_HOST}
    )
    # note: no need to add the host-side libcoi to the install
    # targets; it's already done above
  ENDIF()
ENDIF()

##############################################################
# Configure find_package files
##############################################################

INCLUDE(${CMAKE_SOURCE_DIR}/cmake/ospray_cmake_config.cmake)
