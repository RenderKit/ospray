## ======================================================================== ##
## Copyright 2009-2015 Intel Corporation                                    ##
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

# configure this dir for the proper "OSPRAY_LIB_SUFFIX" etc (depending
# on which target we build for )
CONFIGURE_OSPRAY()

OPTION(OSPRAY_EXP_DATA_PARALLEL "Experimental data-parallel compositing mode")
OPTION(OSPRAY_PIN_ASYNC "Pin async mpi comm threads?" OFF)
MARK_AS_ADVANCED(OSPRAY_PIN_ASYNC)

OPTION(EXP_NEW_BB_VOLUME_KERNELS "Experimental new block/bricked volume layout" OFF)

# -------------------------------------------------------
# Setup tasking system build configuration
# -------------------------------------------------------

SET(OSPRAY_TASKING_SYSTEM TBB CACHE STRING
    "Use TBB or OpenMP as for per-node thread tasking system")
SET_PROPERTY(CACHE OSPRAY_TASKING_SYSTEM PROPERTY STRINGS TBB OpenMP)

IF(${OSPRAY_TASKING_SYSTEM} STREQUAL "TBB")
  SET(USE_TBB TRUE)
ELSE()
  SET(USE_TBB FALSE)
ENDIF()

IF(USE_TBB)
  FIND_PACKAGE(TBB REQUIRED)
  ADD_DEFINITIONS(-DOSPRAY_USE_TBB)
  INCLUDE_DIRECTORIES(${TBB_INCLUDE_DIRS})
ELSE(USE_TBB)
  FIND_PACKAGE(OpenMP)#NOTE(jda) - intentionally optional (fallback w/o TBB)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
ENDIF(USE_TBB)

# -------------------------------------------------------
# Build and configure embree
# -------------------------------------------------------

#NOTE(jda) - Embree assumes that USE_TBB will be defined correctly (above)...
INCLUDE(../cmake/build_embree_2.7.1.cmake)

# -------------------------------------------------------
# Setup ospray source files
# -------------------------------------------------------

SET(OSPRAY_SOURCES
  device/nwlayer.cpp

  math/box.ispc

  common/OSPCommon.ispc
  common/OSPCommon.cpp
  common/Core.cpp
  common/Managed.cpp
  common/ObjectHandle.cpp
  common/Data.cpp
  common/Model.ispc
  common/Model.cpp
  common/Material.cpp
  common/Library.cpp
  common/Thread.cpp

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
  render/obj/OBJRenderer.ispc
  render/obj/OBJMaterial.ispc
  render/obj/OBJRenderer.cpp
  render/obj/OBJMaterial.cpp
  render/raytracer/RaytraceRenderer.ispc
  render/raytracer/RaytraceMaterial.ispc
  render/raytracer/RaytraceMaterial.ih
  render/raytracer/RaytraceRenderer.cpp
  render/raytracer/RaytraceMaterial.cpp
  render/volume/RaycastVolumeMaterial.cpp
  render/volume/RaycastVolumeRenderer.cpp
  render/volume/RaycastVolumeRenderer.ispc
  render/volume/RaycastVolumeRendererMaterial.ih
  render/volume/RaycastVolumeRendererMaterial.ispc

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

  # ============================================
  ADD_EXECUTABLE(ospCreateCompositeTestCubes${OSPRAY_EXE_SUFFIX}
    mpi/testing/createCompositeTestCubes
    )
  TARGET_LINK_LIBRARIES(ospCreateCompositeTestCubes${OSPRAY_EXE_SUFFIX}
    ospray${OSPRAY_LIB_SUFFIX}
    )

  # ============================================
#  INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)
#  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
#  ADD_EXECUTABLE(ospTestDistributedApp${OSPRAY_EXE_SUFFIX}
#    mpi/testing/TestDistributedApp
#    )
#  TARGET_LINK_LIBRARIES(ospTestDistributedApp${OSPRAY_EXE_SUFFIX}
#    ospray${OSPRAY_LIB_SUFFIX}
#    )

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
  SET(OSPRAY_MIC_COI ON)
  INCLUDE_DIRECTORIES(/opt/intel/mic/coi/include)
  INCLUDE_DIRECTORIES(/usr/include/intel-coi)
  IF (THIS_IS_MIC)
    SET(OSPRAY_COI_COMPONENTS api/COIDeviceDev.cpp)
  ELSE()
    SET(OSPRAY_COI_COMPONENTS api/COIDeviceHost.cpp)
  ENDIF()
  SET(OSPRAY_SOURCES ${OSPRAY_SOURCES} ${OSPRAY_COI_COMPONENTS})
ENDIF()

##############################################################
# build the ospray library (using flags and compilers as configured by
# CONFIGURE_OSPRAY() and/or CONFIGURE_MPI()
##############################################################
SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
FIND_PACKAGE(Threads REQUIRED)

OSPRAY_ADD_LIBRARY(ospray${OSPRAY_LIB_SUFFIX} SHARED
  ${OSPRAY_SOURCES}
	)

TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX}
  ospray_embree${OSPRAY_LIB_SUFFIX}
  ${TBB_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
	)

IF (OSPRAY_DISPLAYCLUSTER)
  TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX}
    ${DISPLAYCLUSTER_LIBRARIES}
  )
ENDIF()


# ------------------------------------------------------------
INSTALL(DIRECTORY include/ospray DESTINATION include FILES_MATCHING PATTERN "*.h")
INSTALL(DIRECTORY common DESTINATION include/ospray FILES_MATCHING PATTERN "*.h")
INSTALL(DIRECTORY embree-v2.7.1 DESTINATION include/ospray FILES_MATCHING PATTERN "*.h")
SET_TARGET_PROPERTIES(ospray${OSPRAY_LIB_SUFFIX}
  PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
INSTALL(TARGETS ospray${OSPRAY_LIB_SUFFIX}
  EXPORT ospray${OSPRAY_LIB_SUFFIX}Export
  DESTINATION lib
)
INSTALL(EXPORT ospray${OSPRAY_LIB_SUFFIX}Export
  DESTINATION ${CMAKE_DIR} FILE libospray${OSPRAY_LIB_SUFFIX}Targets.cmake
)
##############################################################



##############################################################
# MPI DEVICE - mpi worker
##############################################################
IF (OSPRAY_MPI)
  TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} ${MPI_CXX_LIBRARIES})

  ADD_EXECUTABLE(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} mpi/MPIWorker.cpp)
  TARGET_LINK_LIBRARIES(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} ospray${OSPRAY_LIB_SUFFIX})
  # ------------------------------------------------------------
  INSTALL(TARGETS ospray_mpi_worker${OSPRAY_EXE_SUFFIX} DESTINATION bin)
ENDIF()


##############################################################
# COI DEVICE
##############################################################
IF (OSPRAY_BUILD_COI_DEVICE)
  IF (THIS_IS_MIC)
    # ------------------------------------------------------------
    # dev-side of COI device: lib dev-side libospray to coi dev libs,...
    # ------------------------------------------------------------
    TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX}
      ${LIBCOI_DEVICE}
      )
    # ... and add the coi worker executable
    ADD_EXECUTABLE(ospray_coi_worker.mic api/COIDeviceWorker.cpp)
    TARGET_LINK_LIBRARIES(ospray_coi_worker.mic
      ospray${OSPRAY_LIB_SUFFIX}
      ${LIBCOI_DEVICE}
      )
    # ------------------------------------------------------------
    INSTALL(TARGETS ospray${OSPRAY_LIB_SUFFIX} DESTINATION lib)
    INSTALL(TARGETS ospray_coi_worker.mic DESTINATION bin)
  ELSE()
    # ------------------------------------------------------------
    # host-side of COI device: just link libospray to coi host libs
    # ------------------------------------------------------------
    TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX}
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
