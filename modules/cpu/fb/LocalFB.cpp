// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LocalFB.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include "ImageOp.h"
#include "SparseFB.h"
#include "fb/LocalFB_ispc.h"
#include "render/util.h"
#include "rkcommon/common.h"
#include "rkcommon/tasking/parallel_for.h"

namespace ospray {

LocalFrameBuffer::LocalFrameBuffer(const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels)
    : AddStructShared(_size, _colorBufferFormat, channels),
      numRenderTasks(divRoundUp(size, getRenderTaskSize())),
      taskErrorRegion(hasVarianceBuffer ? getNumRenderTasks() : vec2i(0))
{
  const size_t pixelBytes = sizeOf(_colorBufferFormat);
  const size_t numPixels = _size.long_product();

  colorBuffer.resize(pixelBytes * numPixels);

  if (hasDepthBuffer)
    depthBuffer.resize(numPixels);

  if (hasAccumBuffer) {
    accumBuffer.resize(numPixels);

    taskAccumID.resize(getTotalRenderTasks(), 0);
  }

  if (hasVarianceBuffer)
    varianceBuffer.resize(numPixels);

  if (hasNormalBuffer)
    normalBuffer.resize(numPixels);

  if (hasAlbedoBuffer)
    albedoBuffer.resize(numPixels);

  if (hasPrimitiveIDBuffer)
    primitiveIDBuffer.resize(numPixels);

  if (hasObjectIDBuffer)
    objectIDBuffer.resize(numPixels);

  if (hasInstanceIDBuffer)
    instanceIDBuffer.resize(numPixels);

  // TODO: Better way to pass the task IDs that doesn't require just storing
  // them all? Maybe as blocks/tiles similar to when we just had tiles? Will
  // make task ID lookup more expensive for sparse case though
  renderTaskIDs.reserve(getTotalRenderTasks());
  for (uint32_t i = 0; i < getTotalRenderTasks(); ++i) {
    renderTaskIDs.push_back(i);
  }

  // TODO: Could use TBB parallel sort here if it's exposed through the rkcommon
  // tasking system
  std::sort(renderTaskIDs.begin(),
      renderTaskIDs.end(),
      [&](const uint32_t &a, const uint32_t &b) {
        const vec2i p_a = getTaskStartPos(a);
        const vec2i p_b = getTaskStartPos(b);
        return interleaveZOrder(p_a.x, p_a.y) < interleaveZOrder(p_b.x, p_b.y);
      });

  getSh()->super.accumulateSample =
      ispc::LocalFrameBuffer_accumulateSample_addr();
  getSh()->super.getRenderTaskDesc =
      ispc::LocalFrameBuffer_getRenderTaskDesc_addr();
  getSh()->super.completeTask = ispc::LocalFrameBuffer_completeTask_addr();

  getSh()->colorBuffer = getDataSafe(colorBuffer);
  getSh()->depthBuffer = getDataSafe(depthBuffer);
  getSh()->accumBuffer = getDataSafe(accumBuffer);
  getSh()->varianceBuffer = getDataSafe(varianceBuffer);
  getSh()->normalBuffer = getDataSafe(normalBuffer);
  getSh()->albedoBuffer = getDataSafe(albedoBuffer);
  getSh()->taskAccumID = getDataSafe(taskAccumID);
  getSh()->taskRegionError = taskErrorRegion.errorBuffer();
  getSh()->numRenderTasks = numRenderTasks;
  getSh()->primitiveIDBuffer = getDataSafe(primitiveIDBuffer);
  getSh()->objectIDBuffer = getDataSafe(objectIDBuffer);
  getSh()->instanceIDBuffer = getDataSafe(instanceIDBuffer);
}

void LocalFrameBuffer::commit()
{
  FrameBuffer::commit();

  imageOps.clear();
  if (imageOpData) {
    FrameBufferView fbv(this,
        getColorBufferFormat(),
        getDataSafe(colorBuffer),
        getDataSafe(depthBuffer),
        getDataSafe(normalBuffer),
        getDataSafe(albedoBuffer));

    for (auto &&obj : *imageOpData)
      imageOps.push_back(obj->attach(fbv));
  }
  prepareImageOps();
}

vec2i LocalFrameBuffer::getNumRenderTasks() const
{
  return numRenderTasks;
}

uint32_t LocalFrameBuffer::getTotalRenderTasks() const
{
  return numRenderTasks.long_product();
}

utility::ArrayView<uint32_t> LocalFrameBuffer::getRenderTaskIDs()
{
  return utility::ArrayView<uint32_t>(renderTaskIDs);
}

std::string LocalFrameBuffer::toString() const
{
  return "ospray::LocalFrameBuffer";
}

void LocalFrameBuffer::clear()
{
  getSh()->super.frameID = -1; // we increment at the start of the frame
  // it is only necessary to reset the accumID,
  // LocalFrameBuffer_accumulateTile takes care of clearing the
  // accumulating buffers
  std::fill(taskAccumID.begin(), taskAccumID.end(), 0);

  // always also clear error buffer (if present)
  if (hasVarianceBuffer) {
    taskErrorRegion.clear();
  }
}

void LocalFrameBuffer::writeTiles(const containers::AlignedVector<Tile> &tiles)
{
  // TODO: The parallel dispatch part of this should be moved into ISPC as an
  // ISPC launch that calls the individual (currently) exported functions that
  // we call below in this loop
  tasking::parallel_for(tiles.size(), [&](const size_t i) {
    const Tile *tile = &tiles[i];
    if (hasDepthBuffer) {
      ispc::LocalFrameBuffer_writeDepthTile(getSh(), tile);
    }

    if (hasAlbedoBuffer) {
      ispc::LocalFrameBuffer_writeAuxTile(getSh(),
          tile,
          (ispc::vec3f *)albedoBuffer.data(),
          tile->ar,
          tile->ag,
          tile->ab);
    }

    if (hasPrimitiveIDBuffer) {
      ispc::LocalFrameBuffer_writeIDTile(
          getSh(), tile, getSh()->primitiveIDBuffer, tile->pid);
    }

    if (hasObjectIDBuffer) {
      ispc::LocalFrameBuffer_writeIDTile(
          getSh(), tile, getSh()->objectIDBuffer, tile->gid);
    }

    if (hasInstanceIDBuffer) {
      ispc::LocalFrameBuffer_writeIDTile(
          getSh(), tile, getSh()->instanceIDBuffer, tile->iid);
    }

    if (hasNormalBuffer)
      ispc::LocalFrameBuffer_writeAuxTile(getSh(),
          tile,
          (ispc::vec3f *)normalBuffer.data(),
          tile->nx,
          tile->ny,
          tile->nz);

    if (!colorBuffer.empty()) {
      switch (getColorBufferFormat()) {
      case OSP_FB_RGBA8:
        ispc::LocalFrameBuffer_writeTile_RGBA8(getSh(), tile);
        break;
      case OSP_FB_SRGBA:
        ispc::LocalFrameBuffer_writeTile_SRGBA(getSh(), tile);
        break;
      case OSP_FB_RGBA32F:
        ispc::LocalFrameBuffer_writeTile_RGBA32F(getSh(), tile);
        break;
      default:
        NOT_IMPLEMENTED;
      }
    }
  });
}

void LocalFrameBuffer::writeTiles(const SparseFrameBuffer *sparseFb)
{
  const auto &tiles = sparseFb->getTiles();
  writeTiles(tiles);

  assert(getRenderTaskSize() == sparseFb->getRenderTaskSize());
  const vec2i renderTaskSize = getRenderTaskSize();

  if (!hasVarianceBuffer) {
    return;
  }

  uint32_t renderTaskID = 0;
  for (size_t i = 0; i < tiles.size(); ++i) {
    const auto &tile = tiles[i];
    const box2i taskRegion(
        tile.region.lower / renderTaskSize, tile.region.upper / renderTaskSize);
    for (int y = taskRegion.lower.y; y < taskRegion.upper.y; ++y) {
      for (int x = taskRegion.lower.x; x < taskRegion.upper.x;
           ++x, ++renderTaskID) {
        const vec2i task(x, y);
        taskErrorRegion.update(task, sparseFb->taskError(renderTaskID));
      }
    }
  }
}

vec2i LocalFrameBuffer::getTaskStartPos(const uint32_t taskID) const
{
  const vec2i numRenderTasks = getNumRenderTasks();
  vec2i taskStart(taskID % numRenderTasks.x, taskID / numRenderTasks.x);
  return taskStart * getRenderTaskSize();
}

float LocalFrameBuffer::taskError(const uint32_t taskID) const
{
  return taskErrorRegion[taskID];
}

void LocalFrameBuffer::beginFrame()
{
  FrameBuffer::beginFrame();

  std::for_each(imageOps.begin(),
      imageOps.end(),
      [](std::unique_ptr<LiveImageOp> &p) { p->beginFrame(); });
}

void LocalFrameBuffer::endFrame(
    const float errorThreshold, const Camera *camera)
{
  if (!imageOps.empty() && firstFrameOperation < imageOps.size()) {
    std::for_each(imageOps.begin() + firstFrameOperation,
        imageOps.end(),
        [&](std::unique_ptr<LiveImageOp> &iop) {
          LiveFrameOp *fop = dynamic_cast<LiveFrameOp *>(iop.get());
          if (fop)
            fop->process(camera);
        });
  }

  std::for_each(imageOps.begin(),
      imageOps.end(),
      [](std::unique_ptr<LiveImageOp> &p) { p->endFrame(); });

  frameVariance = taskErrorRegion.refine(errorThreshold);
}

const void *LocalFrameBuffer::mapBuffer(OSPFrameBufferChannel channel)
{
  const void *buf = nullptr;

  switch (channel) {
  case OSP_FB_COLOR:
    buf = getDataSafe(colorBuffer);
    break;
  case OSP_FB_DEPTH:
    buf = getDataSafe(depthBuffer);
    break;
  case OSP_FB_NORMAL:
    buf = getDataSafe(normalBuffer);
    break;
  case OSP_FB_ALBEDO:
    buf = getDataSafe(albedoBuffer);
    break;
  case OSP_FB_ID_PRIMITIVE:
    buf = getDataSafe(primitiveIDBuffer);
    break;
  case OSP_FB_ID_OBJECT:
    buf = getDataSafe(objectIDBuffer);
    break;
  case OSP_FB_ID_INSTANCE:
    buf = getDataSafe(instanceIDBuffer);
    break;
  default:
    break;
  }

  if (buf)
    this->refInc();

  return buf;
}

void LocalFrameBuffer::unmap(const void *mappedMem)
{
  if (mappedMem)
    this->refDec();
}

} // namespace ospray
