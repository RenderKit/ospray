// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "api/Device.h"
#include "common/MPICommon.h"
#include "common/Managed.h"
#include "common/ObjectHandle.h"
#include "render/DistributedLoadBalancer.h"
#include "render/LoadBalancer.h"
#include "rkcommon/platform.h"

namespace ospray {
namespace mpi {

template <typename OSPRAY_TYPE>
inline OSPRAY_TYPE *lookupDistributedObject(OSPObject obj)
{
  auto &handle = reinterpret_cast<ObjectHandle &>(obj);
  auto *object = (OSPRAY_TYPE *)handle.lookup();

  if (!object) {
    throw std::runtime_error("#dmpi: ObjectHandle doesn't exist!");
  }
  return object;
}

struct MPIDistributedDevice : public api::Device
{
  MPIDistributedDevice();
  ~MPIDistributedDevice() override;

  // ManagedObject Implementation /////////////////////////////////////////

  void commit() override;

  // Device Implementation ////////////////////////////////////////////////

  /*! create a new frame buffer */
  OSPFrameBuffer frameBufferCreate(const vec2i &size,
      const OSPFrameBufferFormat mode,
      const uint32 channels) override;

  /*! create a new transfer function object (out of list of
    registered transfer function types) */
  OSPTransferFunction newTransferFunction(const char *type) override;

  /*! have given renderer create a new Light */
  OSPLight newLight(const char *light_type) override;

  /*! map frame buffer */
  const void *frameBufferMap(
      OSPFrameBuffer fb, OSPFrameBufferChannel channel) override;

  /*! unmap previously mapped frame buffer */
  void frameBufferUnmap(const void *mapped, OSPFrameBuffer fb) override;

  /*! create a new ImageOp object (out of list of registered ImageOps) */
  OSPImageOperation newImageOp(const char *type) override;

  void resetAccumulation(OSPFrameBuffer _fb) override;

  // Instancing ///////////////////////////////////////////////////////////

  OSPGroup newGroup() override;
  OSPInstance newInstance(OSPGroup group) override;

  // Top-level Worlds /////////////////////////////////////////////////////

  OSPWorld newWorld() override;
  box3f getBounds(OSPObject) override;

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

  OSPRenderer newRenderer(const char *type) override;

  OSPGeometry newGeometry(const char *type) override;

  OSPMaterial newMaterial(const char *material_type) override;

  OSPCamera newCamera(const char *type) override;

  OSPVolume newVolume(const char *type) override;

  OSPGeometricModel newGeometricModel(OSPGeometry geom) override;

  OSPVolumetricModel newVolumetricModel(OSPVolume volume) override;

  OSPFuture renderFrame(
      OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld) override;

  int isReady(OSPFuture, OSPSyncEvent) override;

  void wait(OSPFuture, OSPSyncEvent) override;

  void cancel(OSPFuture) override;

  float getProgress(OSPFuture) override;

  float getTaskDuration(OSPFuture) override;

  float getVariance(OSPFrameBuffer) override;

  void *getPostProcessingCommandQueuePtr() override;

  /*! load module */
  int loadModule(const char *name) override;

  // Object + Parameter Lifetime Management ///////////////////////////////

  void setObjectParam(OSPObject object,
      const char *name,
      OSPDataType type,
      const void *mem) override;

  void removeObjectParam(OSPObject object, const char *name) override;

  void commit(OSPObject object) override;
  void release(OSPObject _obj) override;
  void retain(OSPObject _obj) override;

  /*! create a new Texture object */
  OSPTexture newTexture(const char *type) override;

  OSPPickResult pick(
      OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld, const vec2f &) override;

 private:
  ObjectHandle allocateHandle();

  bool initialized{false};
  bool shouldFinalizeMPI{false};

  std::shared_ptr<Device> internalDevice = nullptr;
};

} // namespace mpi
} // namespace ospray
