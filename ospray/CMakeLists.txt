# #####################################################################
# INTEL CORPORATION PROPRIETARY INFORMATION                            
# This software is supplied under the terms of a license agreement or  
# nondisclosure agreement with Intel Corporation and may not be copied 
# or disclosed except in accordance with the terms of that agreement.  
# Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
# #####################################################################


# configure this dir for the proper "OSPRAY_LIB_SUFFIX" etc (depending
# on which target we build for )
CONFIGURE_OSPRAY()

# build and configure embree
INCLUDE(../cmake/build_embree.cmake)

SET(OSPRAY_SOURCES
  device/nwlayer.cpp

  common/ospcommon.cpp
  common/ospcommon.ispc
  common/managed.cpp
  common/data.cpp
  common/model.cpp
  common/model.ispc
  common/material.cpp
  common/library.cpp

  fb/swapchain.cpp 
  fb/framebuffer.ispc
  fb/framebuffer.cpp
  fb/localfb.ispc

  camera/camera.cpp
  camera/perspectivecamera.ispc
  camera/perspectivecamera.cpp

  volume/volume.ispc
  volume/volume.cpp
  volume/naive32.ispc
  volume/naive32.cpp
  #	volume/naive64.ispc
  #	volume/naive64.cpp
  volume/bricked32.ispc
  volume/bricked32.cpp
  volume/bricked64.ispc
  volume/bricked64.cpp

  transferfunction/TransferFunction.cpp
  transferfunction/TransferFunctionPiecewiseLinear.ispc
  transferfunction/TransferFunctionPiecewiseLinear.cpp

  geometry/geometry.ispc
  geometry/geometry.cpp
  geometry/trianglemesh.ispc
  geometry/trianglemesh.cpp
  geometry/streamlines.cpp
  geometry/streamlines.ispc
  geometry/instance.ispc
  geometry/instance.cpp
  geometry/spheres.cpp
  geometry/spheres.ispc
  geometry/cylinders.cpp
  geometry/cylinders.ispc

  lights/light.cpp
  lights/light.ispc
  lights/pointlight.cpp
  lights/pointlight.ispc
  lights/directionallight.cpp
  lights/directionallight.ispc
  lights/spotlight.cpp
  lights/spotlight.ispc

  texture/texture2d.cpp
  texture/texture2d.ispc
  
  render/loadbalancer.cpp
  render/renderer.ispc
  render/renderer.cpp
  render/util.ispc
  render/raycast/raycast.cpp
  render/raycast/raycast.ispc
  render/ao16/ao16.cpp
  render/ao16/ao16.ispc
  render/obj/objrenderer.ispc
  render/obj/objmaterial.ispc
  render/obj/objrenderer.cpp
  render/obj/objmaterial.cpp
  render/obj/objpointlight.ispc
  render/obj/objpointlight.cpp
  render/obj/objspotlight.ispc
  render/obj/objspotlight.cpp

  api/api.cpp
  api/device.cpp
  api/localdevice.cpp
  api/handle.cpp
  )

# -------------------------------------------------------
# MPI components
# -------------------------------------------------------
IF (OSPRAY_MPI)
  # if mpi mode is enabled, we have to configure the right mpi
  # compiler etc.
  CONFIGURE_MPI()

  SET(OSPRAY_SOURCES ${OSPRAY_SOURCES}   
    mpi/mpidevice.cpp
    mpi/mpicommon.cpp
    mpi/mpiloadbalancer.cpp
    mpi/worker.cpp
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
    SET(OSPRAY_COI_COMPONENTS api/coidevice_dev.cpp)
  ELSE()
    ADD_DEFINITIONS(-DOSPRAY_COI_DEV=0)
    ADD_DEFINITIONS(-DOSPRAY_COI_HOST=1)
    SET(OSPRAY_COI_COMPONENTS api/coidevice_host.cpp)
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
  ADD_EXECUTABLE(ospray_mpi_worker${OSPRAY_EXE_SUFFIX} mpi/mpiworker.cpp)
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
    ADD_EXECUTABLE(ospray_coi_worker.mic api/coidevice_worker.cpp)
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








