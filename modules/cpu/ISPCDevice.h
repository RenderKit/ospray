// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "Device.h"
// device run-time
#include "DeviceRT.h"
// embree
#include "common/Embree.h"
#ifdef OSPRAY_ENABLE_VOLUMES
// openvkl
#include "openvkl/openvkl.h"
// comment break to prevent clang-format from reordering openvkl includes
#include "openvkl/device/openvkl.h"
#endif

/*! \file ISPCDevice.h Implements the "local" device for local rendering */

#ifdef OSPRAY_TARGET_SYCL
namespace ispc {
int ISPCDevice_programCount();
int ISPCDevice_isa();
} // namespace ispc
#endif

namespace ospray {

struct LocalTiledLoadBalancer;
struct MipMapCache;

namespace api {

struct OSPRAY_SDK_INTERFACE ISPCDevice : public Device
{
  ISPCDevice();
  ISPCDevice(std::unique_ptr<devicert::Device> device);
  virtual ~ISPCDevice() override;

  /////////////////////////////////////////////////////////////////////////
  // ManagedObject Implementation /////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void commit() override;

  /////////////////////////////////////////////////////////////////////////
  // Device Implementation ////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  int loadModule(const char *name) override;

  // OSPRay Data Arrays ///////////////////////////////////////////////////

  OSPData newSharedData(const void *sharedData,
      OSPDataType,
      const vec3ul &numItems,
      const vec3l &byteStride,
      OSPDeleterCallback,
      const void *userPtr) override;

  OSPData newData(OSPDataType, const vec3ul &numItems) override;

  void copyData(const OSPData source,
      OSPData destination,
      const vec3ul &destinationIndex) override;

  // Renderable Objects ///////////////////////////////////////////////////

  OSPLight newLight(const char *type) override;

  OSPCamera newCamera(const char *type) override;

  OSPGeometry newGeometry(const char *type) override;
  OSPVolume newVolume(const char *type) override;

  OSPGeometricModel newGeometricModel(OSPGeometry geom) override;
  OSPVolumetricModel newVolumetricModel(OSPVolume volume) override;

  // Model Meta-Data //////////////////////////////////////////////////////

  OSPMaterial newMaterial(const char *material_type) override;

  OSPTransferFunction newTransferFunction(const char *type) override;

  OSPTexture newTexture(const char *type) override;

  // Instancing ///////////////////////////////////////////////////////////

  OSPGroup newGroup() override;
  OSPInstance newInstance(OSPGroup group) override;

  // Top-level Worlds /////////////////////////////////////////////////////

  OSPWorld newWorld() override;
  box3f getBounds(OSPObject) override;

  // Object + Parameter Lifetime Management ///////////////////////////////

  void setObjectParam(OSPObject object,
      const char *name,
      OSPDataType type,
      const void *mem) override;

  void removeObjectParam(OSPObject object, const char *name) override;

  void commit(OSPObject object) override;
  void release(OSPObject _obj) override;
  void retain(OSPObject _obj) override;

  // FrameBuffer Manipulation /////////////////////////////////////////////

  OSPFrameBuffer frameBufferCreate(const vec2i &size,
      const OSPFrameBufferFormat mode,
      const uint32 channels) override;

  OSPImageOperation newImageOp(const char *type) override;

  const void *frameBufferMap(
      OSPFrameBuffer fb, const OSPFrameBufferChannel) override;

  void frameBufferUnmap(const void *mapped, OSPFrameBuffer fb) override;

  float getVariance(OSPFrameBuffer) override;

  void resetAccumulation(OSPFrameBuffer _fb) override;

  // Frame Rendering //////////////////////////////////////////////////////

  OSPRenderer newRenderer(const char *type) override;

  OSPFuture renderFrame(
      OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld) override;

  int isReady(OSPFuture, OSPSyncEvent) override;
  void wait(OSPFuture, OSPSyncEvent) override;
  void cancel(OSPFuture) override;
  float getProgress(OSPFuture) override;
  float getTaskDuration(OSPFuture) override;

  OSPPickResult pick(
      OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld, const vec2f &) override;

  std::shared_ptr<LocalTiledLoadBalancer> loadBalancer;

  RTCDevice getEmbreeDevice()
  {
    return embreeDevice;
  }

#ifdef OSPRAY_ENABLE_VOLUMES
  VKLDevice getVklDevice()
  {
    return vklDevice;
  }
#endif

  inline devicert::Device &getDRTDevice()
  {
    return *drtDevice;
  }

#ifdef OSPRAY_TARGET_SYCL
  /* Compute the rounded dispatch global size for the given work group size.
   * SYCL requires that globalSize % workgroupSize == 0, this function will
   * round up globalSize and return nd_range(roundedSize, workgroupSize).
   * The kernel being launched must discard tasks that are out of bounds
   * bounds due to this rounding
   */
  sycl::nd_range<1> computeDispatchRange(
      const size_t globalSize, const size_t workgroupSize) const;
#endif

  inline MipMapCache &getMipMapCache()
  {
    return *mipMapCache;
  }

 private:
  std::unique_ptr<devicert::Device> drtDevice;

  RTCDevice embreeDevice = nullptr;
#ifdef OSPRAY_ENABLE_VOLUMES
  VKLDevice vklDevice = nullptr;
#endif

  // External SYCL context and device
  void *appSyclCtx{nullptr};
  void *appSyclDevice{nullptr};

  // Device-wide MIP map cache is needed because the same texture data objects
  // (and thus generated MIP maps) can be shared among many textures
  std::unique_ptr<MipMapCache> mipMapCache;
};

} // namespace api
} // namespace ospray
