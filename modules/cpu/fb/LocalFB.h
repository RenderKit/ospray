// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "common/ISPCRTBuffers.h"
#include "fb/FrameBuffer.h"
// rkcommon
#include "rkcommon/containers/AlignedVector.h"
#include "rkcommon/utility/ArrayView.h"
// ispc shared
#include "LocalFBShared.h"
#include "TileShared.h"

namespace ospray {

struct SparseFrameBuffer;
struct LiveVarianceFrameOp;
struct LiveColorConversionFrameOp;

struct OSPRAY_SDK_INTERFACE LocalFrameBuffer
    : public AddStructShared<FrameBuffer, ispc::LocalFB>
{
  LocalFrameBuffer(api::ISPCDevice &device,
      const vec2i &size,
      ColorBufferFormat colorBufferFormat,
      const uint32 channels);

  virtual ~LocalFrameBuffer() override;

  virtual void commit() override;

  // Return the number of render tasks in the x and y direction
  // This is the kernel launch dims to render the image
  virtual vec2i getNumRenderTasks() const override;

  virtual uint32_t getTotalRenderTasks() const override;

  virtual utility::ArrayView<uint32_t> getRenderTaskIDs(
      const float errorThreshold, const uint32_t spp) override;

  virtual float getVariance() const override;

  // common function to help printf-debugging, every derived class should
  // override this!
  virtual std::string toString() const override;

  AsyncEvent postProcess(bool wait) override;

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
  void writeTiles(SparseFrameBuffer *sparseFb);

  // NOTE: All per-pixel data is only allocated if the corresponding channel
  //       flag was passed on construction

  // one RGBA per pixel
  std::unique_ptr<BufferDeviceShadowed<vec4f>> colorBuffer;
  // one RGBA per pixel, post-processing output
  std::unique_ptr<BufferDeviceShadowed<vec4f>> ppColorBuffer;
  // one RGBA per pixel, does not accumulate all samples, for variance
  // estimation
  std::unique_ptr<BufferDevice<vec4f>> varianceBuffer;
  // one float per pixel
  std::unique_ptr<BufferDeviceShadowed<float>> depthBuffer;
  // accumulated world-space normal per pixel
  std::unique_ptr<BufferDeviceShadowed<vec3f>> normalBuffer;
  // accumulated, one RGB per pixel
  std::unique_ptr<BufferDeviceShadowed<vec3f>> albedoBuffer;
  // primitive ID, object ID, and instance ID
  std::unique_ptr<BufferDeviceShadowed<uint32_t>> primitiveIDBuffer;
  std::unique_ptr<BufferDeviceShadowed<uint32_t>> objectIDBuffer;
  std::unique_ptr<BufferDeviceShadowed<uint32_t>> instanceIDBuffer;

 protected:
  vec2i getTaskStartPos(const uint32_t taskID) const;

  //// Data ////

  api::ISPCDevice &device;

  vec2i numRenderTasks;

  std::unique_ptr<BufferDeviceShadowed<uint32_t>> renderTaskIDs;
  std::unique_ptr<BufferDeviceShadowed<uint32_t>> activeTaskIDs;

  // Array of frame operations
  std::vector<std::unique_ptr<LiveFrameOpInterface>> frameOps;

  // Variance frame operation
  std::unique_ptr<LiveVarianceFrameOp> varianceFrameOp;

  // Color conversion frame operation
  std::unique_ptr<LiveColorConversionFrameOp> colorConversionFrameOp;

 private:
  float errorThreshold; // remember
  // Not used, to be removed after mpi module refactor
  float taskError(const uint32_t) const override
  {
    return 0.f;
  }
};

} // namespace ospray
