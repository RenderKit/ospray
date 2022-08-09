// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SparseFB.h"
#include <rkcommon/tasking/parallel_for.h>
#include <rkcommon/utility/ArrayView.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "ImageOp.h"
#include "common/BufferShared.h"
#ifndef OSPRAY_TARGET_DPCPP
#include "fb/SparseFB_ispc.h"
#endif
#include "render/util.h"
#include "rkcommon/common.h"

namespace ospray {

SparseFrameBuffer::SparseFrameBuffer(api::ISPCDevice &device,
    const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels,
    const std::vector<uint32_t> &_tileIDs,
    const bool overrideUseTaskAccumIDs)
    : AddStructShared(
        device.getIspcrtDevice(), device, _size, _colorBufferFormat, channels),
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

SparseFrameBuffer::SparseFrameBuffer(api::ISPCDevice &device,
    const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels,
    const bool overrideUseTaskAccumIDs)
    : AddStructShared(
        device.getIspcrtDevice(), device, _size, _colorBufferFormat, channels),
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
  return utility::ArrayView<uint32_t>(
      renderTaskIDs->data(), renderTaskIDs->size());
}

std::string SparseFrameBuffer::toString() const
{
  return "ospray::SparseFrameBuffer";
}

float SparseFrameBuffer::taskError(const uint32_t taskID) const
{
  // TODO: When called? USM thrashing
  return (*taskErrorBuffer)[taskID];
}

void SparseFrameBuffer::setTaskError(const uint32_t taskID, const float error)
{
  // TODO: When called? USM thrashing
  (*taskErrorBuffer)[taskID] = error;
}

void SparseFrameBuffer::setTaskAccumID(const uint32_t taskID, const int accumID)
{
  // TODO: When called? USM thrashing
  (*taskAccumID)[taskID] = accumID;
}

void SparseFrameBuffer::beginFrame()
{
  FrameBuffer::beginFrame();

  // TODO: USM thrashing! We should launch a kernel here!
  for (auto &tile : *tiles) {
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
  // TODO: Should be done in a kernel or with a GPU memcpy, USM thrashing
  std::fill(taskAccumID->begin(), taskAccumID->end(), 0);

  // also clear the task error buffer if present
  if (hasVarianceBuffer) {
    std::fill(taskErrorBuffer->begin(), taskErrorBuffer->end(), inf);
  }
}

const utility::ArrayView<Tile> SparseFrameBuffer::getTiles() const
{
  return utility::ArrayView<Tile>(tiles->data(), tiles->size());
}

const utility::ArrayView<uint32_t> SparseFrameBuffer::getTileIDs() const
{
  return utility::ArrayView<uint32_t>(tileIDs->data(), tileIDs->size());
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
  tileIDs = make_buffer_shared_unique<uint32_t>(
      getISPCDevice().getIspcrtDevice(), _tileIDs.size());
  std::memcpy(
      tileIDs->data(), _tileIDs.data(), sizeof(uint32_t) * _tileIDs.size());
  numRenderTasks =
      vec2i(tileIDs->size() * TILE_SIZE, TILE_SIZE) / getRenderTaskSize();

  if (hasVarianceBuffer) {
    taskErrorBuffer = make_buffer_shared_unique<float>(
        getISPCDevice().getIspcrtDevice(), numRenderTasks.long_product());
    std::fill(taskErrorBuffer->begin(), taskErrorBuffer->end(), inf);
  }

  tiles = make_buffer_shared_unique<Tile>(
      getISPCDevice().getIspcrtDevice(), tileIDs->size());
  const vec2f rcpSize = rcp(vec2f(size));
  for (size_t i = 0; i < tileIDs->size(); ++i) {
    vec2i tilePos;
    const uint32_t tid = (*tileIDs)[i];
    tilePos.x = tid % totalTiles.x;
    tilePos.y = tid / totalTiles.x;

    Tile &t = (*tiles)[i];
    t.fbSize = size;
    t.rcp_fbSize = rcpSize;
    t.region.lower = tilePos * TILE_SIZE;
    t.region.upper = min(t.region.lower + TILE_SIZE, size);
    t.accumID = 0;
  }

  const size_t numPixels = tileIDs->size() * TILE_SIZE * TILE_SIZE;
  if (hasVarianceBuffer) {
    varianceBuffer = make_buffer_shared_unique<vec4f>(
        getISPCDevice().getIspcrtDevice(), numPixels);
  }

  if (hasAccumBuffer) {
    accumulationBuffer = make_buffer_shared_unique<vec4f>(
        getISPCDevice().getIspcrtDevice(), numPixels);
  }

  if (hasAccumBuffer || useTaskAccumIDs) {
    taskAccumID = make_buffer_shared_unique<int>(
        getISPCDevice().getIspcrtDevice(), getTotalRenderTasks());
    std::memset(taskAccumID->begin(), 0, taskAccumID->size() * sizeof(int));
  }

  // TODO: Should find a better way for allowing sparse task id sets
  // here we have this array b/c the tasks will be filtered down based on
  // variance termination
  renderTaskIDs = make_buffer_shared_unique<uint32_t>(
      getISPCDevice().getIspcrtDevice(), getTotalRenderTasks());
  for (uint32_t i = 0; i < getTotalRenderTasks(); ++i) {
    (*renderTaskIDs)[i] = i;
  }

  const uint32_t nTasksPerTile = getNumTasksPerTile();

  // Sort each tile's tasks in Z order
  rkcommon::tasking::parallel_for(tiles->size(), [&](const size_t i) {
    std::sort(renderTaskIDs->begin() + i * nTasksPerTile,
        renderTaskIDs->begin() + (i + 1) * nTasksPerTile,
        [&](const uint32_t &a, const uint32_t &b) {
          const vec2i p_a = getTaskPosInTile(a);
          const vec2i p_b = getTaskPosInTile(b);
          return interleaveZOrder(p_a.x, p_a.y)
              < interleaveZOrder(p_b.x, p_b.y);
        });
  });

  clear();

#ifndef OSPRAY_TARGET_DPCPP
  getSh()->super.accumulateSample =
      reinterpret_cast<ispc::FrameBuffer_accumulateSampleFct>(
          ispc::SparseFrameBuffer_accumulateSample_addr());
  getSh()->super.getRenderTaskDesc =
      reinterpret_cast<ispc::FrameBuffer_getRenderTaskDescFct>(
          ispc::SparseFrameBuffer_getRenderTaskDesc_addr());
  getSh()->super.completeTask =
      reinterpret_cast<ispc::FrameBuffer_completeTaskFct>(
          ispc::SparseFrameBuffer_completeTask_addr());
#endif

  getSh()->numRenderTasks = numRenderTasks;
  getSh()->totalTiles = totalTiles;

  getSh()->tiles = tiles->data();
  getSh()->numTiles = tiles->size();

  getSh()->taskAccumID = taskAccumID ? taskAccumID->data() : nullptr;
  getSh()->accumulationBuffer =
      accumulationBuffer ? accumulationBuffer->data() : nullptr;
  getSh()->varianceBuffer = varianceBuffer ? varianceBuffer->data() : nullptr;
  getSh()->taskRegionError =
      taskErrorBuffer ? taskErrorBuffer->data() : nullptr;
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
