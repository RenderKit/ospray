// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SparseFB.h"
#include <rkcommon/tasking/parallel_for.h>
#include <cstdlib>
#include "ImageOp.h"
#include "fb/SparseFB_ispc.h"
#include "render/util.h"
#include "rkcommon/common.h"

namespace ospray {

SparseFrameBuffer::SparseFrameBuffer(const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels,
    const std::vector<uint32_t> &_tileIDs,
    const bool overrideUseTaskAccumIDs)
    : AddStructShared(_size, _colorBufferFormat, channels),
      useTaskAccumIDs((channels & OSP_FB_ACCUM) || overrideUseTaskAccumIDs),
      totalTiles(divRoundUp(size, vec2i(TILE_SIZE)))
{
  if (size.x <= 0 || size.y <= 0) {
    throw std::runtime_error(
        "local framebuffer has invalid size. Dimensions must be greater than "
        "0");
  }

  if (!_tileIDs.empty()) {
    setTiles(_tileIDs);
  }
}

SparseFrameBuffer::SparseFrameBuffer(const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels,
    const bool overrideUseTaskAccumIDs)
    : AddStructShared(_size, _colorBufferFormat, channels),
      useTaskAccumIDs((channels & OSP_FB_ACCUM) || overrideUseTaskAccumIDs),
      totalTiles(divRoundUp(size, vec2i(TILE_SIZE)))
{
  if (size.x <= 0 || size.y <= 0) {
    throw std::runtime_error(
        "local framebuffer has invalid size. Dimensions must be greater than "
        "0");
  }
}

void SparseFrameBuffer::commit()
{
  FrameBuffer::commit();

  imageOps.clear();
  if (imageOpData) {
    FrameBufferView fbv(
        this, getColorBufferFormat(), nullptr, nullptr, nullptr, nullptr);

    // Sparse framebuffer cannot execute frame operations because it doesn't
    // have the full framebuffer. This is handled by the parent object managing
    // the set of sparse framebuffer's, so here we just ignore them
    for (auto &&obj : *imageOpData) {
      if (dynamic_cast<PixelOp *>(obj)) {
        imageOps.push_back(obj->attach(fbv));
      }
    }
  }
  prepareImageOps();
}

vec2i SparseFrameBuffer::getNumRenderTasks() const
{
  return numRenderTasks;
}

uint32_t SparseFrameBuffer::getTotalRenderTasks() const
{
  return numRenderTasks.product();
}

utility::ArrayView<uint32_t> SparseFrameBuffer::getRenderTaskIDs()
{
  return utility::ArrayView<uint32_t>(renderTaskIDs);
}

std::string SparseFrameBuffer::toString() const
{
  return "ospray::SparseFrameBuffer";
}

float SparseFrameBuffer::taskError(const uint32_t taskID) const
{
  return taskErrorBuffer[taskID];
}

void SparseFrameBuffer::setTaskError(const uint32_t taskID, const float error)
{
  taskErrorBuffer[taskID] = error;
}

void SparseFrameBuffer::setTaskAccumID(const uint32_t taskID, const int accumID)
{
  taskAccumID[taskID] = accumID;
}

void SparseFrameBuffer::beginFrame()
{
  FrameBuffer::beginFrame();

  for (auto &tile : tiles) {
    tile.accumID = getFrameID();
  }

  std::for_each(imageOps.begin(),
      imageOps.end(),
      [](std::unique_ptr<LiveImageOp> &p) { p->beginFrame(); });
}

void SparseFrameBuffer::endFrame(const float, const Camera *)
{
  std::for_each(imageOps.begin(),
      imageOps.end(),
      [](std::unique_ptr<LiveImageOp> &p) { p->endFrame(); });
}

const void *SparseFrameBuffer::mapBuffer(OSPFrameBufferChannel)
{
  return nullptr;
}

void SparseFrameBuffer::unmap(const void *) {}

void SparseFrameBuffer::clear()
{
  // we increment at the start of the frame
  getSh()->super.frameID = -1;

  // We only need to reset the accumID, SparseFB_accumulateWriteSample will
  // handle overwriting the image when accumID == 0
  std::fill(taskAccumID.begin(), taskAccumID.end(), 0);

  // also clear the task error buffer if present
  if (hasVarianceBuffer) {
    std::fill(taskErrorBuffer.begin(), taskErrorBuffer.end(), inf);
  }
}

const containers::AlignedVector<Tile> &SparseFrameBuffer::getTiles() const
{
  return tiles;
}

const std::vector<uint32_t> &SparseFrameBuffer::getTileIDs() const
{
  return tileIDs;
}

uint32_t SparseFrameBuffer::getTileIndexForTask(uint32_t taskID) const
{
  // Find which tile this task falls into
  // tileIdx -> index in the SparseFB's list of tiles
  return taskID / getNumTasksPerTile();
}

void SparseFrameBuffer::setTiles(const std::vector<uint32_t> &_tileIDs)
{
  // (Re-)configure the sparse framebuffer based on the tileIDs we're passed
  tileIDs = _tileIDs;
  numRenderTasks =
      vec2i(tileIDs.size() * TILE_SIZE, TILE_SIZE) / getRenderTaskSize();

  if (hasVarianceBuffer) {
    taskErrorBuffer.resize(numRenderTasks.long_product());
    std::fill(taskErrorBuffer.begin(), taskErrorBuffer.end(), inf);
  }

  tiles.resize(tileIDs.size());
  const vec2f rcpSize = rcp(vec2f(size));
  for (size_t i = 0; i < tileIDs.size(); ++i) {
    vec2i tilePos;
    tilePos.x = tileIDs[i] % totalTiles.x;
    tilePos.y = tileIDs[i] / totalTiles.x;

    tiles[i].fbSize = size;
    tiles[i].rcp_fbSize = rcpSize;
    tiles[i].region.lower = tilePos * TILE_SIZE;
    tiles[i].region.upper = min(tiles[i].region.lower + TILE_SIZE, size);
    tiles[i].accumID = 0;
  }

  const size_t numPixels = tileIDs.size() * TILE_SIZE * TILE_SIZE;
  if (hasVarianceBuffer) {
    varianceBuffer.resize(numPixels);
  }

  if (hasAccumBuffer) {
    accumulationBuffer.resize(numPixels);
  }

  if (hasAccumBuffer || useTaskAccumIDs) {
    taskAccumID.resize(getTotalRenderTasks(), 0);
  }

  // TODO: Should find a better way for allowing sparse task id sets
  // here we have this array b/c the tasks will be filtered down based on
  // variance termination
  renderTaskIDs.resize(getTotalRenderTasks());
  for (uint32_t i = 0; i < getTotalRenderTasks(); ++i) {
    renderTaskIDs[i] = i;
  }

  const uint32_t nTasksPerTile = getNumTasksPerTile();

  // Sort each tile's tasks in Z order
  rkcommon::tasking::parallel_for(tiles.size(), [&](const size_t i) {
    std::sort(renderTaskIDs.begin() + i * nTasksPerTile,
        renderTaskIDs.begin() + (i + 1) * nTasksPerTile,
        [&](const uint32_t &a, const uint32_t &b) {
          const vec2i p_a = getTaskPosInTile(a);
          const vec2i p_b = getTaskPosInTile(b);
          return interleaveZOrder(p_a.x, p_a.y)
              < interleaveZOrder(p_b.x, p_b.y);
        });
  });

  clear();

  getSh()->super.accumulateSample =
      ispc::SparseFrameBuffer_accumulateSample_addr();
  getSh()->super.getRenderTaskDesc =
      ispc::SparseFrameBuffer_getRenderTaskDesc_addr();
  getSh()->super.completeTask = ispc::SparseFrameBuffer_completeTask_addr();

  getSh()->numRenderTasks = numRenderTasks;
  getSh()->totalTiles = totalTiles;

  getSh()->tiles = tiles.data();
  getSh()->numTiles = tiles.size();

  getSh()->taskAccumID = getDataSafe(taskAccumID);
  getSh()->accumulationBuffer = getDataSafe(accumulationBuffer);
  getSh()->varianceBuffer = getDataSafe(varianceBuffer);
  getSh()->taskRegionError = getDataSafe(taskErrorBuffer);
}

vec2i SparseFrameBuffer::getTaskPosInTile(const uint32_t taskID) const
{
  // Find where this task is supposed to render within this tile
  const vec2i tasksPerTile = vec2i(TILE_SIZE) / getRenderTaskSize();
  const uint32 taskTileID = taskID % (tasksPerTile.x * tasksPerTile.y);
  vec2i taskStart(taskTileID % tasksPerTile.x, taskTileID / tasksPerTile.x);
  return taskStart * getRenderTaskSize();
}

uint32_t SparseFrameBuffer::getNumTasksPerTile() const
{
  const vec2i tileDims(TILE_SIZE);
  const vec2i tasksPerTile = tileDims / getRenderTaskSize();
  return tasksPerTile.long_product();
}

} // namespace ospray
