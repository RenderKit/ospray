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

IF  ((${OSPRAY_XEON_TARGET} STREQUAL "AVX2") AND
		(${OSPRAY_COMPILER} STREQUAL "GCC") AND
		(NOT APPLE)
		)
  MESSAGE(WARNING "Warning: You have selected 'AVX2' target and 'GCC'-compiler, but many older gcc versions do not yet support AVX2. If you run into build issues, please switch to either 'ICC'-compiler, or upgrade your gcc version")
ENDIF()

# build and configure embree
INCLUDE(../cmake/build_embree.cmake)
INCLUDE_DIRECTORIES_ISPC(${CMAKE_CURRENT_SOURCE_DIR}/render/pathtracer)

SET(OSPRAY_SOURCES
  device/nwlayer.cpp

  common/OSPCommon.cpp
  common/OSPCommon.ispc
  common/Managed.cpp
  common/Data.cpp
  common/Model.cpp
  common/Model.ispc
  common/Material.cpp
  common/Library.cpp

  fb/FrameBuffer.ispc
  fb/FrameBuffer.cpp
  fb/LocalFB.ispc

  camera/Camera.cpp
  camera/PerspectiveCamera.ispc
  camera/PerspectiveCamera.cpp

  volume/BlockBrickedVolume.cpp
  volume/BlockBrickedVolume.ispc
  volume/GridAccelerator.ispc
  volume/StructuredVolume.cpp
  volume/SymbolRegistry.cpp
  volume/Volume.cpp

  transferFunction/LinearTransferFunction.ispc
  transferFunction/LinearTransferFunction.cpp
  transferFunction/TransferFunction.cpp
  transferFunction/TransferFunction.ispc
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

  lights/Light.cpp
  lights/Light.ispc
  lights/PointLight.cpp
  lights/PointLight.ispc
  lights/DirectionalLight.cpp
  lights/DirectionalLight.ispc
  lights/SpotLight.cpp
  lights/SpotLight.ispc
  lights/SymbolRegistry.cpp

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
  render/obj/OBJPointLight.ispc
  render/obj/OBJPointLight.cpp
  render/obj/OBJSpotLight.ispc
  render/obj/OBJSpotLight.cpp
  render/volume/RaycastVolumeRenderer.cpp
  render/volume/RaycastVolumeRenderer.ispc
  render/volume/SymbolRegistry.cpp

  render/pathtracer/PathTracer.ispc
  render/pathtracer/PathTracer.cpp
  render/pathtracer/lights/Light.ispc
  render/pathtracer/lights/AmbientLight.ispc
  render/pathtracer/lights/DistantLight.ispc
  render/pathtracer/lights/PointLight.ispc
  render/pathtracer/lights/SpotLight.ispc
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
    )

  ADD_EXECUTABLE(ospTestMPIAsyncMessaging${OSPRAY_EXE_SUFFIX}
    mpi/testing/TestAsync
    )
  TARGET_LINK_LIBRARIES(ospTestMPIAsyncMessaging${OSPRAY_EXE_SUFFIX}
    ospray${OSPRAY_LIB_SUFFIX}
    )
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
OSPRAY_ADD_LIBRARY(ospray${OSPRAY_LIB_SUFFIX} SHARED 
  ${OSPRAY_SOURCES}
  )
TARGET_LINK_LIBRARIES(ospray${OSPRAY_LIB_SUFFIX} ospray_embree${OSPRAY_LIB_SUFFIX} pthread dl)

# ------------------------------------------------------------
INSTALL(DIRECTORY include/ospray DESTINATION include FILES_MATCHING PATTERN "*.h")
SET_TARGET_PROPERTIES(ospray${OSPRAY_LIB_SUFFIX} 
  PROPERTIES VERSION ${OSPRAY_VERSION} SOVERSION ${OSPRAY_SOVERSION})
INSTALL(TARGETS ospray${OSPRAY_LIB_SUFFIX} DESTINATION lib)
##############################################################



##############################################################
# MPI DEVICE - mpi worker
##############################################################
IF (OSPRAY_MPI)
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

