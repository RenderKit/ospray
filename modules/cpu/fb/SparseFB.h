// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDevice.h"
#include "fb/FrameBuffer.h"
#include "fb/LocalFB.h"
#include "fb/TaskError.h"
#include "rkcommon/containers/AlignedVector.h"
#include "rkcommon/utility/ArrayView.h"
// ispc shared
#include "TileShared.h"
#include "common/ISPCRTBuffers.h"
#include "fb/SparseFBShared.h"

namespace ospray {
/* The sparse framebuffer stores a subset of tiles of a larger framebuffer
 * to allow rendering tasks to directly write to this allocated tile memory,
 * rather than requiring on the fly allocation of tiles. The sparse framebuffer
 * is used as a component of a "parent" framebuffer that is responsible for
 * distributing the full image among different sparse framebuffers and
 * collecting the tiles from them into a complete image in some way.
 *
 * The sparse framebuffer doesn't store the entire image data and thus doesn't
 * support frame/tile operations or mapping.
 */
struct OSPRAY_SDK_INTERFACE SparseFrameBuffer
    : public AddStructShared<FrameBuffer, ispc::SparseFB>
{
  /* Create a sparse framebuffer holding the tiles specified in tileIDs
   * overrideUseTaskAccumIDs specifies whether task accumIDs should still
   * be used even if the sparsefb is not performing accumulation buffering
   */
  SparseFrameBuffer(api::ISPCDevice &device,
      const vec2i &size,
      ColorBufferFormat colorBufferFormat,
      const uint32 channels,
      const std::vector<uint32_t> &tileIDs,
      const bool overrideUseTaskAccumIDs = false);

  /* Create a sparse framebuffer that stores no image tiles. Tiles can be added
   * to the empty sparse framebuffer by calling setTiles
   * overrideUseTaskAccumIDs specifies whether task accumIDs should still
   * be used even if the sparsefb is not performing accumulation buffering
   */
  SparseFrameBuffer(api::ISPCDevice &device,
      const vec2i &size,
      ColorBufferFormat colorBufferFormat,
      const uint32 channels,
      const bool overrideUseTaskAccumIDs = false);

  virtual void commit() override;

  // Return the number of render tasks in the x and y direction
  // This is the kernel launch dims to render the image
  virtual vec2i getNumRenderTasks() const override;

  virtual uint32_t getTotalRenderTasks() const override;

  virtual utility::ArrayView<uint32_t> getRenderTaskIDs(
      float errorThreshold) override;

  //! \brief common function to help printf-debugging
  /*! \detailed Every derived class should override this! */
  virtual std::string toString() const override;

  float taskError(const uint32_t taskID) const override;

  /* Set the error for a given task, used by the parent framebuffer managing the
   * sparse framebuffers to sync error computed over the entire image
   */
  void setTaskError(const uint32_t taskID, const float error);

  void beginFrame() override;

  void endFrame(const float, const Camera *) override {}

  AsyncEvent postProcess(const Camera *, bool) override
  {
    AsyncEvent e;
    // Do not run post-processing on sparse frame buffer
    return e;
  }

  // Mapping sparse framebuffers is not supported, will return null
  const void *mapBuffer(OSPFrameBufferChannel channel) override;

  void unmap(const void *mappedMem) override;

  void clear() override;

  size_t getNumTiles() const;

  const utility::ArrayView<Tile> getTiles();

  // Get a view of the device memory of the tiles
  const utility::ArrayView<Tile> getTilesDevice() const;

  const utility::ArrayView<uint32_t> getTileIDs();

  // Get the index of the tile in the tile ID and Tiles lists that this task
  // falls into
  uint32_t getTileIndexForTask(uint32_t taskID) const;

  /* Change the tiles stored in this SparseFrameBuffer, re-using the existing
   * storage. New tiles will be allocated if the size of the tileIDs passed
   * exceeds those currently stored in the SparseFrameBuffer
   */
  void setTiles(
      const std::vector<uint32_t> &tileIDs, const int initialTaskAccumID = 0);

  // Return the image region for the tile
  box2i getTileRegion(uint32_t tileID) const;

  //// Data ////
 protected:
  // Get the position of a task in its tile
  vec2i getTaskPosInTile(const uint32_t taskID) const;

  uint32_t getNumTasksPerTile() const;

  api::ISPCDevice &device;

  // The tiles in this framebuffer
  std::unique_ptr<BufferDeviceShadowed<Tile>> tiles;
  // Track if we need to read back tiles to the host
  bool tilesDirty = false;

  // Accumulation buffer for the tile colors for accumulation buffering. The
  // rgba data in the Tiles stores the final color for display in float format,
  // while the accumulation buffer is used for progressive refinement
  std::unique_ptr<BufferDevice<vec4f>> accumulationBuffer;

  // Variance data for the image, stored in tiled order with one RGBA value per
  // pixel, accumulates every other sample, for variance estimation
  std::unique_ptr<BufferDevice<vec4f>> varianceBuffer;

  // holds accumID per render task, for adaptive accumulation
  std::unique_ptr<BufferDeviceShadowed<int>> taskAccumID;

  // The sparsefb can also use task accum IDs without accumulation buffering,
  // when it's being used as a component of another framebuffer.
  // this is true by default when created with OSP_FB_ACCUM
  bool useTaskAccumIDs;

  // Does this need to be USM at all?
  std::vector<uint32_t> tileIDs;

  // Total number of tiles that the framebuffer is divided into, including those
  // not owned by this sparsefb
  vec2i totalTiles;

  // Total number of render tasks that the framebuffer is divided into,
  // including those not owned by this sparsefb
  vec2i numRenderTasks{0};

  // holds error per task for each tile, stored in tiled order.
  // The SparseFB doesn't do its own error refinement since it doesn't have
  // access to error data for the entire framebuffer
  std::unique_ptr<BufferShared<float>> taskErrorBuffer;

  std::unique_ptr<BufferDeviceShadowed<uint32_t>> renderTaskIDs;
  std::unique_ptr<BufferDeviceShadowed<uint32_t>> activeTaskIDs;
};
} // namespace ospray
