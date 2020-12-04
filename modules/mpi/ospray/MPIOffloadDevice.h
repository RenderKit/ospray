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

  /*! Send work will use the write command function to pack the command into the
   * command buffer. The writeCmd method is called twice, once to compute the
   * size being written and a second time to output the data to the writer.
   */
  template <typename Fcn>
  void sendWork(const Fcn &writeCmd, bool submitImmediately);

  /*! The data shared with the application. The workerType may not be the same
   * as the type of the data, because managed objects are unknown on the
   * application rank, and are represented just as uint64 handles. Thus the
   * local data objects see them as OSP_ULONG types, while the worker type
   * is the true managed object type
   */
  struct ApplicationData
  {
    Data *data = nullptr;
    // We can't tell the local Data object that the contained type
    // is a managed type, because all the head node has are object handles
    OSPDataType workerType = OSP_UNKNOWN;
    bool releaseHazard = false;
  };

  /*! Send data work should be called by data transfer writeCmd lambdas, to
   * insert inline data or start the data transfer for large data. If doing
   * a large data transfer the transfer is started on the second time the
   * lambda is called (i.e., when writing to the command buffer, not when
   * computing the size)
   */
  void sendDataWork(
      rkcommon::networking::WriteStream &writer, ApplicationData &appData);

  void submitWork();

  int rootWorkerRank() const;

  ObjectHandle allocateHandle() const;

  /*! @{ read and write stream for the work commands */
  std::unique_ptr<rkcommon::networking::Fabric> fabric;

  using FrameBufferMapping = std::unique_ptr<utility::OwnedArray<uint8_t>>;

  std::unordered_map<int64_t, FrameBufferMapping> framebufferMappings;

  std::unordered_map<int64_t, ApplicationData> sharedData;

  std::unordered_set<int64_t> futures;

  uint32_t maxCommandBufferEntries;
  uint32_t commandBufferSize;
  uint32_t maxInlineDataSize;

  size_t nBufferedCommands = 0;

  rkcommon::networking::FixedBufferWriter commandBuffer;

  bool initialized{false};

#ifdef ENABLE_PROFILING
  mpicommon::ProfilingPoint masterStart;
#endif
};

template <typename Fcn>
void MPIOffloadDevice::sendWork(const Fcn &writeCmd, bool submitImmediately)
{
  networking::WriteSizeCalculator sizeCalc;
  writeCmd(sizeCalc);

  // Note: curious if this ever happens, with a reasonable command buffer size
  // (anything >= 1MB) I don't think this should be an issue since commands are
  // quite small, and limiting the inline data size to be half the command
  // buffer size should avoid this.
  if (sizeCalc.writtenSize >= commandBuffer.capacity()) {
    throw std::runtime_error("Work size is too large for command buffer!");
  }
  if (sizeCalc.writtenSize >= commandBuffer.available()) {
    submitWork();
  }

  const size_t dbgWriteStart = commandBuffer.cursor;

  writeCmd(commandBuffer);

  postStatusMsg(OSP_LOG_DEBUG)
      << "#osp.mpi.app: buffering command: "
      << work::tagName(*(reinterpret_cast<work::TAG *>(
             commandBuffer.buffer->begin() + dbgWriteStart)));

  ++nBufferedCommands;

  if (submitImmediately || nBufferedCommands >= maxCommandBufferEntries) {
    submitWork();
  }
}

template <typename T>
void MPIOffloadDevice::setParam(ObjectHandle obj,
    const char *param,
    const void *_val,
    const OSPDataType type)
{
  const T &val = *(const T *)_val;
  sendWork(
      [&](rkcommon::networking::WriteStream &writer) {
        writer << work::SET_PARAM << obj.i64 << param << type << val;
      },
      false);
}

} // namespace mpi
} // namespace ospray
