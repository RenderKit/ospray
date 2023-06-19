// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SparseFB.h"
#include "PixelOp.h"
#include "fb/FrameBufferView.h"
#include "render/util.h"
#include "rkcommon/common.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/utility/ArrayView.h"
#ifndef OSPRAY_TARGET_SYCL
#include "fb/SparseFB_ispc.h"
#endif

#include <cstdlib>
#include <numeric>

namespace ospray {

SparseFrameBuffer::SparseFrameBuffer(api::ISPCDevice &device,
    const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels,
    const std::vector<uint32_t> &_tileIDs,
    const bool overrideUseTaskAccumIDs)
    : AddStructShared(device.getIspcrtContext(),
        device,
        _size,
        _colorBufferFormat,
        channels,
        FFO_FB_SPARSE),
      useTaskAccumIDs((channels & OSP_FB_ACCUM) || overrideUseTaskAccumIDs),
      totalTiles(divRoundUp(size, vec2i(TILE_SIZE)))
{
  if (size.x <= 0 || size.y <= 0) {
    throw std::runtime_error(
        "local framebuffer has invalid size. Dimensions must be greater than "
        "0");
  }

  setTiles(_tileIDs);
}

SparseFrameBuffer::SparseFrameBuffer(api::ISPCDevice &device,
    const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels,
    const bool overrideUseTaskAccumIDs)
    : AddStructShared(device.getIspcrtContext(),
        device,
        _size,
        _colorBufferFormat,
        channels,
        FFO_FB_SPARSE),
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

  if (imageOpData) {
    FrameBufferView fbv(this,
        getColorBufferFormat(),
        getNumPixels(),
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    // Sparse framebuffer cannot execute frame operations because it doesn't
    // have the full framebuffer. This is handled by the parent object managing
    // the set of sparse framebuffer's, so here we just ignore them
    prepareLiveOpsForFBV(fbv, false, true);
  }
}

vec2i SparseFrameBuffer::getNumRenderTasks() const
{
  return numRenderTasks;
}

uint32_t SparseFrameBuffer::getTotalRenderTasks() const
{
  return numRenderTasks.product();
}

utility::ArrayView<uint32_t> SparseFrameBuffer::getRenderTaskIDs(
    float errorThreshold)
{
  if (!renderTaskIDs)
    return utility::ArrayView<uint32_t>(nullptr, 0);

  if (errorThreshold > 0.0f && hasVarianceBuffer) {
    auto last = std::copy_if(renderTaskIDs->begin(),
        renderTaskIDs->end(),
        activeTaskIDs->begin(),
        [=](uint32_t i) { return taskError(i) > errorThreshold; });
    return utility::ArrayView<uint32_t>(
        activeTaskIDs->data(), last - activeTaskIDs->begin());
  } else
    return utility::ArrayView<uint32_t>(
        renderTaskIDs->data(), renderTaskIDs->size());
}

std::string SparseFrameBuffer::toString() const
{
  return "ospray::SparseFrameBuffer";
}

float SparseFrameBuffer::taskError(const uint32_t taskID) const
{
  // If this SparseFB doesn't have any tiles return 0. This should not typically
  // be called in this case anyways
  if (!tiles) {
    return 0.f;
  }

  if (!taskErrorBuffer) {
    throw std::runtime_error(
        "SparseFrameBuffer::taskError: trying to get task error on FB without variance/error buffers");
  }
  // Note: USM migration?
  return (*taskErrorBuffer)[taskID];
}

void SparseFrameBuffer::setTaskError(const uint32_t taskID, const float error)
{
  // If this SparseFB doesn't have any tiles then do nothing. This should not
  // typically be called in this case anyways
  if (!tiles) {
    return;
  }
  if (!taskErrorBuffer) {
    throw std::runtime_error(
        "SparseFrameBuffer::setTaskError: trying to set task error on FB without variance/error buffers");
  }
  (*taskErrorBuffer)[taskID] = error;
}

void SparseFrameBuffer::setTaskAccumID(const uint32_t taskID, const int accumID)
{
  // Note: USM migration?
  if (taskAccumID) {
    (*taskAccumID)[taskID] = accumID;
  } else if (tiles) {
    // If we have tiles but not an accum buffer it's a hard error to call this
    // function. If we don't have tiles we just exit silently, this function
    // shouldn't be called when this SparseFB is empty anyways
    throw std::runtime_error(
        "SparseFrameBuffer::setTaskAccumID: called on SparseFB without accumIDs");
  }
}

void SparseFrameBuffer::beginFrame()
{
  FrameBuffer::beginFrame();

  // TODO We could launch a kernel here
  if (tiles) {
    for (auto &tile : *tiles) {
      tile.accumID = getFrameID();
    }
  }
}

const void *SparseFrameBuffer::mapBuffer(OSPFrameBufferChannel)
{
  return nullptr;
}

void SparseFrameBuffer::unmap(const void *) {}

void SparseFrameBuffer::clear()
{
  FrameBuffer::clear();

  // We only need to reset the accumID, SparseFB_accumulateWriteSample will
  // handle overwriting the image when accumID == 0
  // TODO: Should be done in a kernel or with a GPU memcpy, USM thrashing
  if (taskAccumID) {
    std::fill(taskAccumID->begin(), taskAccumID->end(), 0);
  }

  // also clear the task error buffer if present
  if (taskErrorBuffer) {
    std::fill(taskErrorBuffer->begin(), taskErrorBuffer->end(), inf);
  }
}

const utility::ArrayView<Tile> SparseFrameBuffer::getTiles() const
{
  if (!tiles) {
    return utility::ArrayView<Tile>(nullptr, 0);
  }
  return utility::ArrayView<Tile>(tiles->data(), tiles->size());
}

const utility::ArrayView<uint32_t> SparseFrameBuffer::getTileIDs() const
{
  if (!tileIDs) {
    return utility::ArrayView<uint32_t>(nullptr, 0);
  }
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
  if (!_tileIDs.empty()) {
    tileIDs = make_buffer_shared_unique<uint32_t>(
        getISPCDevice().getIspcrtContext(), _tileIDs.size());
    std::memcpy(
        tileIDs->data(), _tileIDs.data(), sizeof(uint32_t) * _tileIDs.size());
    numRenderTasks =
        vec2i(tileIDs->size() * TILE_SIZE, TILE_SIZE) / getRenderTaskSize();
  } else {
    tileIDs = nullptr;
    numRenderTasks = vec2i(0);
  }

  if (hasVarianceBuffer && !_tileIDs.empty()) {
    taskErrorBuffer = make_buffer_shared_unique<float>(
        getISPCDevice().getIspcrtContext(), numRenderTasks.long_product());
    std::fill(taskErrorBuffer->begin(), taskErrorBuffer->end(), inf);
  } else {
    taskErrorBuffer = nullptr;
  }

  if (!_tileIDs.empty()) {
    tiles = make_buffer_shared_unique<Tile>(
        getISPCDevice().getIspcrtContext(), tileIDs->size());
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
  } else {
    tiles = nullptr;
  }

  const size_t numPixels = tiles ? tileIDs->size() * TILE_SIZE * TILE_SIZE : 0;
  if (hasVarianceBuffer && !_tileIDs.empty()) {
    varianceBuffer = make_buffer_shared_unique<vec4f>(
        getISPCDevice().getIspcrtContext(), numPixels);
  } else {
    varianceBuffer = nullptr;
  }

  if (hasAccumBuffer && !_tileIDs.empty()) {
    accumulationBuffer = make_buffer_shared_unique<vec4f>(
        getISPCDevice().getIspcrtContext(), numPixels);
  } else {
    accumulationBuffer = nullptr;
  }

  if ((hasAccumBuffer || useTaskAccumIDs) && !_tileIDs.empty()) {
    taskAccumID = make_buffer_shared_unique<int>(
        getISPCDevice().getIspcrtContext(), getTotalRenderTasks());
    std::memset(taskAccumID->begin(), 0, taskAccumID->size() * sizeof(int));
  } else {
    taskAccumID = nullptr;
  }

  // TODO: Should find a better way for allowing sparse task id sets
  // here we have this array b/c the tasks will be filtered down based on
  // variance termination
  if (!_tileIDs.empty()) {
    renderTaskIDs = make_buffer_shared_unique<uint32_t>(
        getISPCDevice().getIspcrtContext(), getTotalRenderTasks());
    std::iota(renderTaskIDs->begin(), renderTaskIDs->end(), 0);
  } else {
    renderTaskIDs = nullptr;
  }
  if (hasVarianceBuffer && !_tileIDs.empty()) {
    activeTaskIDs = make_buffer_shared_unique<uint32_t>(
        getISPCDevice().getIspcrtContext(), getTotalRenderTasks());
  } else {
    activeTaskIDs = nullptr;
  }

  const uint32_t nTasksPerTile = getNumTasksPerTile();

  // Sort each tile's tasks in Z order
  if (tiles) {
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
  }

  clear();

#ifndef OSPRAY_TARGET_SYCL
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

  getSh()->tiles = tiles ? tiles->data() : nullptr;
  getSh()->numTiles = tiles ? tiles->size() : 0;

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
