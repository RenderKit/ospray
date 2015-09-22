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

SET(OSPRAY_EXP_COMPOSITING OFF CACHE BOOL "Experimental compositing")
IF (OSPRAY_EXP_COMPOSITING)

  SET(OSPRAY_EXP_ALPHA_BLENDING OFF CACHE BOOL "Experimental alpha-blending for compositing")
  SET(OSPRAY_EXP_DISTRIBUTED_VOLUME OFF CACHE BOOL "Experimental support for distributed volumes")
ENDIF()

# build and configure embree
INCLUDE(../cmake/build_embree.cmake)
#INCLUDE_DIRECTORIES_ISPC(${CMAKE_CURRENT_SOURCE_DIR}/render/pathtracer)

#ADD_DEFINITIONS(-DTILE_SIZE=${OSPRAY_TILE_SIZE})
#ADD_DEFINITIONS_ISPC(-DTILE_SIZE=${OSPRAY_TILE_SIZE})

SET(OSPRAY_SOURCES
  device/nwlayer.cpp

  math/box.ispc

  common/OSPCommon.ispc
  common/OSPCommon.cpp
  common/Core.cpp
  common/Managed.cpp
  common/Data.cpp
  common/Model.ispc
  common/Model.cpp
  common/Material.cpp
  common/Library.cpp
  common/TaskSys.cpp
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
  volume/GridAccelerator.ispc
  volume/SharedStructuredVolume.ispc
  volume/SharedStructuredVolume.cpp
  volume/StructuredVolume.ispc
  volume/StructuredVolume.cpp
  volume/SymbolRegistry.cpp
  volume/Volume.ispc
  volume/Volume.cpp
	volume/DataDistributedBlockedVolume.cpp

  transferFunction/LinearTransferFunction.ispc
  transferFunction/LinearTransferFunction.cpp
  transferFunction/TransferFunction.ispc
  transferFunction/TransferFunction.cpp
  transferFunction/SymbolRegistry.cpp

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
  render/volume/RaycastVolumeRenderer.cpp
  render/volume/RaycastVolumeRenderer.ispc
  render/volume/RaycastVolumeRendererMaterial.ispc
  render/volume/SymbolRegistry.cpp

  render/pathtracer/PathTracer.ispc
  render/pathtracer/PathTracer.cpp
  render/pathtracer/materials/Material.ispc
  render/pathtracer/materials/OBJ.ispc
  render/pathtracer/materials/OBJ.cpp
  render/pathtracer/materials/Velvet.ispc
  render/pathtracer/materials/Velvet.cpp
  render/pathtracer/materials/Metal.ispc
  render/pathtracer/materials/Metal.cpp
  render/pathtracer/materials/ThinDielectric.ispc
  render/pathtracer/materials/ThinDielectric.cpp
  render/pathtracer/materials/Dielectric.ispc
  render/pathtracer/materials/Dielectric.cpp
  render/pathtracer/materials/MetallicPaint.ispc
  render/pathtracer/materials/MetallicPaint.cpp
  render/pathtracer/materials/Plastic.ispc
  render/pathtracer/materials/Plastic.cpp
  render/pathtracer/materials/Matte.ispc
  render/pathtracer/materials/Matte.cpp

  api/API.cpp
  api/Device.cpp
  api/LocalDevice.cpp
  api/Handle.cpp
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
  INCLUDE(${PROJECT_SOURCE_DIR}/cmake/glut.cmake)
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/ospray/include)
  ADD_EXECUTABLE(ospTestDistributedApp${OSPRAY_EXE_SUFFIX}
    mpi/testing/TestDistributedApp
    )
  TARGET_LINK_LIBRARIES(ospTestDistributedApp${OSPRAY_EXE_SUFFIX}
    ospray${OSPRAY_LIB_SUFFIX}
    )

ENDIF()

# -------------------------------------------------------
# DisplayCluster components
# -------------------------------------------------------
INCLUDE(${PROJECT_SOURCE_DIR}/cmake/displaycluster.cmake)

IF (OSPRAY_DISPLAYCLUSTER)
  CONFIGURE_DISPLAYCLUSTER()
  ADD_DEFINITIONS(-DOSPRAY_DISPLAY_CLUSTER=1)
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
  ADD_DEFINITIONS(-DOSPRAY_MIC_COI=1)
  INCLUDE_DIRECTORIES(/opt/intel/mic/coi/include)
  INCLUDE_DIRECTORIES(/usr/include/intel-coi)
  IF (THIS_IS_MIC)
    ADD_DEFINITIONS(-DOSPRAY_COI_DEV=1)
    ADD_DEFINITIONS(-DOSPRAY_COI_HOST=0)
    SET(OSPRAY_COI_COMPONENTS api/COIDeviceDev.cpp)
  ELSE()
    ADD_DEFINITIONS(-DOSPRAY_COI_DEV=0)
    ADD_DEFINITIONS(-DOSPRAY_COI_HOST=1)
    SET(OSPRAY_COI_COMPONENTS api/COIDeviceHost.cpp)
  ENDIF()
  SET(OSPRAY_SOURCES ${OSPRAY_SOURCES} ${OSPRAY_COI_COMPONENTS})
ELSE()
  SET(OSPRAY_COI_COMPONENTS "")
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

IF (OSPRAY_ALLOW_EXTERNAL_EMBREE)
  LINK_DIRECTORIES(${OSPRAY_EMBREE_SOURCE_DIR})
  LINK_DIRECTORIES(${OSPRAY_EMBREE_SOURCE_DIR}/bin)
  MESSAGE("LINKING EXTENRALLY TO EMBREE IN ${OSPRAY_EMBREE_SOURCE_DIR}")
ENDIF()

TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} 
  ospray_embree${OSPRAY_LIB_SUFFIX} 
  ${CMAKE_THREAD_LIBS_INIT} 
  ${CMAKE_DL_LIBS})

IF (OSPRAY_DISPLAYCLUSTER)
  TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} 
    ${DISPLAYCLUSTER_LIBRARIES})
ENDIF()


# ------------------------------------------------------------
INSTALL(DIRECTORY include/ospray DESTINATION include FILES_MATCHING PATTERN "*.h")
INSTALL(DIRECTORY common DESTINATION include/ospray FILES_MATCHING PATTERN "*.h")
INSTALL(DIRECTORY embree DESTINATION include/ospray FILES_MATCHING PATTERN "*.h")
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
  TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} ${MPI_LIBRARIES})
  # iw - we don't actually need this for intel mpi - "-mt_mpi" does the trick 

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
