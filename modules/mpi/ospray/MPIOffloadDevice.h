// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <unordered_set>
#include "api/Device.h"
#include "common/MPICommon.h"
#include "common/Managed.h"
#include "common/OSPWork.h"
#include "common/Profiling.h"
#include "common/SocketBcastFabric.h"
#include "rkcommon/utility/FixedArray.h"

/*! \file MPIDevice.h Implements the "mpi" device for mpi rendering */

namespace ospray {
namespace mpi {

struct MPIOffloadDevice : public api::Device
{
  MPIOffloadDevice() = default;
  ~MPIOffloadDevice() override;

  /////////////////////////////////////////////////////////////////////////
  // ManagedObject Implementation /////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void commit() override;

  /////////////////////////////////////////////////////////////////////////
  // Device Implementation ////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  int loadModule(const char *name) override;

  // Renderable Objects ///////////////////////////////////////////////////

  OSPLight newLight(const char *type) override;
  OSPCamera newCamera(const char *type) override;
  OSPGeometry newGeometry(const char *type) override;
  OSPVolume newVolume(const char *type) override;

  OSPGeometricModel newGeometricModel(OSPGeometry geom) override;
  OSPVolumetricModel newVolumetricModel(OSPVolume volume) override;

  // Model Meta-Data //////////////////////////////////////////////////////

  OSPMaterial newMaterial(
      const char *renderer_type, const char *material_type) override;

  OSPTransferFunction newTransferFunction(const char *type) override;

  OSPTexture newTexture(const char *type) override;

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
      const vec3l &byteStride) override;

  OSPData newData(OSPDataType, const vec3ul &numItems) override;

  void copyData(const OSPData source,
      OSPData destination,
      const vec3ul &destinationIndex) override;

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

 private:
  void initializeDevice();

  template <typename T>
  void setParam(ObjectHandle obj,
      const char *param,
      const void *_val,
      const OSPDataType tag);

  void sendWork(
      const std::shared_ptr<rkcommon::utility::AbstractArray<uint8_t>> &work,
      bool submitImmediately);

  void sendDataWork(rkcommon::networking::BufferWriter &writer,
      const std::shared_ptr<rkcommon::utility::AbstractArray<uint8_t>> &data);

  void submitWork();

  int rootWorkerRank() const;

  ObjectHandle allocateHandle() const;

  /*! @{ read and write stream for the work commands */
  std::unique_ptr<rkcommon::networking::Fabric> fabric;

  using FrameBufferMapping = std::unique_ptr<utility::OwnedArray<uint8_t>>;

  std::unordered_map<int64_t, FrameBufferMapping> framebufferMappings;

  std::unordered_map<int64_t, std::shared_ptr<utility::AbstractArray<uint8_t>>>
      sharedData;

  std::unordered_set<int64_t> futures;

  uint32_t maxBufferedCommands = 8192;
  uint32_t commandBufferSize = 512 * 1024 * 1024;
  uint32_t maxInlineDataSize = 4 * 1024 * 1024;
  std::shared_ptr<rkcommon::utility::FixedArray<uint8_t>> commandBuffer =
      nullptr;

  uint64_t commandBufferCursor = 0;
  size_t bufferedCommands = 0;
  bool sharedDataViewHazard = false;

  bool initialized{false};

#ifdef ENABLE_PROFILING
  mpicommon::ProfilingPoint masterStart;
#endif
};

template <typename T>
void MPIOffloadDevice::setParam(ObjectHandle obj,
    const char *param,
    const void *_val,
    const OSPDataType type)
{
  const T &val = *(const T *)_val;
  networking::BufferWriter writer;
  writer << work::SET_PARAM << obj.i64 << std::string(param) << type << val;
  sendWork(writer.buffer, false);
}

} // namespace mpi
} // namespace ospray
