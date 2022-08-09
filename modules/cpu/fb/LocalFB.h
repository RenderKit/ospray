// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/utility/ArrayView.h>
#ifdef OSPRAY_TARGET_DPCPP
#include <CL/sycl.hpp>
#endif

// ospray
#include "common/BufferShared.h"
#include "fb/FrameBuffer.h"
#include "fb/TaskError.h"
// rkcommon
#include "rkcommon/containers/AlignedVector.h"
// ispc shared
#include "LocalFBShared.h"
#include "TileShared.h"

namespace ospray {

struct SparseFrameBuffer;

struct OSPRAY_SDK_INTERFACE LocalFrameBuffer
    : public AddStructShared<FrameBuffer, ispc::LocalFB>
{
  LocalFrameBuffer(api::ISPCDevice &device,
      const vec2i &size,
      ColorBufferFormat colorBufferFormat,
      const uint32 channels);

  virtual ~LocalFrameBuffer() override = default;

  virtual void commit() override;

#ifdef OSPRAY_TARGET_DPCPP
  // void setGPUFunctionPtrs(sycl::queue &syclQueue) override;
#endif

  // Return the number of render tasks in the x and y direction
  // This is the kernel launch dims to render the image
  virtual vec2i getNumRenderTasks() const override;

  virtual uint32_t getTotalRenderTasks() const override;

  virtual utility::ArrayView<uint32_t> getRenderTaskIDs() override;

  // common function to help printf-debugging, every derived class should
  // override this!
  virtual std::string toString() const override;

  float taskError(const uint32_t taskID) const override;

  void beginFrame() override;

  void endFrame(const float errorThreshold, const Camera *camera) override;

  const void *mapBuffer(OSPFrameBufferChannel channel) override;

  void unmap(const void *mappedMem) override;

  void clear() override;

  /* Write the tile into this framebuffer's row-major storage. This does not
   * perform accumulation or buffering, data is taken directly from the tile and
   * written to the row-major image stored by the LocalFrameBuffer
   * Safe to call in parallel from multiple threads, as long as each thread is
   * writing different tiles
   */
  void writeTiles(const utility::ArrayView<Tile> &tiles);

  /* Write the tiles of the sparse fb into this framebuffer's row-major storage.
   * Will also copy error data from the sparseFb the full framebuffer task error
   * buffer, if variance buffer is enabled.
   * Safe to call in parallel from multiple threads, as long as each thread is
   * writing different tiles
   */
  void writeTiles(const SparseFrameBuffer *sparseFb);

  // NOTE: All per-pixel data is only allocated if the corresponding channel
  //       flag was passed on construction

  // TODO/NOTE: we use unique ptr here because the default Array constructor
  // allocates a single element "array". We probably want to change this in
  // ispcrt to have behavior more like std::vector/other containers

  // format depends on FrameBuffer::colorBufferFormat
  std::unique_ptr<BufferShared<uint8_t>> colorBuffer;
  // one float per pixel
  std::unique_ptr<BufferShared<float>> depthBuffer;
  // one RGBA per pixel
  std::unique_ptr<BufferShared<vec4f>> accumBuffer;
  // one RGBA per pixel, accumulates every other sample, for variance estimation
  std::unique_ptr<BufferShared<vec4f>> varianceBuffer;
  // accumulated world-space normal per pixel
  std::unique_ptr<BufferShared<vec3f>> normalBuffer;
  // accumulated, one RGB per pixel
  std::unique_ptr<BufferShared<vec3f>> albedoBuffer;
  // primitive ID, object ID, and instance ID
  std::unique_ptr<BufferShared<uint32_t>> primitiveIDBuffer;
  std::unique_ptr<BufferShared<uint32_t>> objectIDBuffer;
  std::unique_ptr<BufferShared<uint32_t>> instanceIDBuffer;

 protected:
  vec2i getTaskStartPos(const uint32_t taskID) const;

  //// Data ////

  vec2i numRenderTasks;

  std::unique_ptr<BufferShared<uint32_t>> renderTaskIDs;
  // holds accumID per render task, for adaptive accumulation
  std::unique_ptr<BufferShared<int32_t>> taskAccumID;

  // holds error per tile and adaptive regions
  TaskError taskErrorRegion;
};

} // namespace ospray
