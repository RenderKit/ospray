// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LocalFB.h"
#include "FrameOp.h"
#include "SparseFB.h"
#include "fb/FrameBufferView.h"
#include "render/util.h"
#include "rkcommon/common.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/utility/ArrayView.h"

#ifndef OSPRAY_TARGET_SYCL
#include "fb/LocalFB_ispc.h"
#else
namespace ispc {

SYCL_EXTERNAL void LocalFrameBuffer_writeTile_RGBA8(
    void *_fb, const void *_tile);
SYCL_EXTERNAL void LocalFrameBuffer_writeTile_SRGBA(
    void *_fb, const void *_tile);
SYCL_EXTERNAL void LocalFrameBuffer_writeTile_RGBA32F(
    void *_fb, const void *_tile);

SYCL_EXTERNAL
void LocalFrameBuffer_writeDepthTile(void *_fb, const void *uniform _tile);

SYCL_EXTERNAL
void LocalFrameBuffer_writeAuxTile(void *_fb,
    const void *_tile,
    void *aux,
    const void *_ax,
    const void *_ay,
    const void *_az);

SYCL_EXTERNAL
void LocalFrameBuffer_writeIDTile(void *uniform _fb,
    const void *uniform _tile,
    uniform uint32 *uniform dst,
    const void *uniform src);
} // namespace ispc
#endif

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>

namespace ospray {

LocalFrameBuffer::LocalFrameBuffer(api::ISPCDevice &device,
    const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels)
    : AddStructShared(device.getIspcrtContext(),
        device,
        _size,
        _colorBufferFormat,
        channels,
        FFO_FB_LOCAL),
      device(device),
      numRenderTasks(divRoundUp(size, getRenderTaskSize())),
      taskErrorRegion(device.getIspcrtContext(),
          hasVarianceBuffer ? getNumRenderTasks() : vec2i(0))
{
  const size_t pixelBytes = sizeOf(_colorBufferFormat);
  const size_t numPixels = _size.long_product();

  if (getColorBufferFormat() != OSP_FB_NONE) {
    colorBuffer = make_buffer_device_shadowed_unique<uint8_t>(
        device.getIspcrtDevice(), pixelBytes * numPixels);
  }

  if (hasDepthBuffer)
    depthBuffer = make_buffer_device_shadowed_unique<float>(
        device.getIspcrtDevice(), numPixels);

  if (hasAccumBuffer) {
    accumBuffer =
        make_buffer_device_unique<vec4f>(device.getIspcrtDevice(), numPixels);

    // TODO: Implement fill function in ISPCRT
    taskAccumID = make_buffer_device_shadowed_unique<int32_t>(
        device.getIspcrtDevice(), getTotalRenderTasks());
    std::memset(taskAccumID->data(), 0, taskAccumID->size() * sizeof(int32_t));
    device.getIspcrtQueue().copyToDevice(*taskAccumID);
  }

  if (hasVarianceBuffer)
    varianceBuffer =
        make_buffer_device_unique<vec4f>(device.getIspcrtDevice(), numPixels);

  if (hasNormalBuffer)
    normalBuffer = make_buffer_device_shadowed_unique<vec3f>(
        device.getIspcrtDevice(), numPixels);

  if (hasAlbedoBuffer)
    albedoBuffer = make_buffer_device_shadowed_unique<vec3f>(
        device.getIspcrtDevice(), numPixels);

  if (hasPrimitiveIDBuffer)
    primitiveIDBuffer = make_buffer_device_shadowed_unique<uint32_t>(
        device.getIspcrtDevice(), numPixels);

  if (hasObjectIDBuffer)
    objectIDBuffer = make_buffer_device_shadowed_unique<uint32_t>(
        device.getIspcrtDevice(), numPixels);

  if (hasInstanceIDBuffer)
    instanceIDBuffer = make_buffer_device_shadowed_unique<uint32_t>(
        device.getIspcrtDevice(), numPixels);

  // TODO: Better way to pass the task IDs that doesn't require just storing
  // them all? Maybe as blocks/tiles similar to when we just had tiles? Will
  // make task ID lookup more expensive for sparse case though
  renderTaskIDs = make_buffer_shared_unique<uint32_t>(
      device.getIspcrtContext(), getTotalRenderTasks());
  std::iota(renderTaskIDs->begin(), renderTaskIDs->end(), 0);
  if (hasVarianceBuffer)
    activeTaskIDs = make_buffer_shared_unique<uint32_t>(
        device.getIspcrtContext(), getTotalRenderTasks());

  // TODO: Could use TBB parallel sort here if it's exposed through the rkcommon
  // tasking system
  std::sort(renderTaskIDs->begin(),
      renderTaskIDs->end(),
      [&](const uint32_t &a, const uint32_t &b) {
        const vec2i p_a = getTaskStartPos(a);
        const vec2i p_b = getTaskStartPos(b);
        return interleaveZOrder(p_a.x, p_a.y) < interleaveZOrder(p_b.x, p_b.y);
      });

#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.accumulateSample =
      reinterpret_cast<ispc::FrameBuffer_accumulateSampleFct>(
          ispc::LocalFrameBuffer_accumulateSample_addr());
  getSh()->super.getRenderTaskDesc =
      reinterpret_cast<ispc::FrameBuffer_getRenderTaskDescFct>(
          ispc::LocalFrameBuffer_getRenderTaskDesc_addr());
  getSh()->super.completeTask =
      reinterpret_cast<ispc::FrameBuffer_completeTaskFct>(
          ispc::LocalFrameBuffer_completeTask_addr());
#endif

  getSh()->colorBuffer = colorBuffer ? colorBuffer->devicePtr() : nullptr;
  getSh()->depthBuffer = depthBuffer ? depthBuffer->devicePtr() : nullptr;
  getSh()->accumBuffer = accumBuffer ? accumBuffer->devicePtr() : nullptr;
  getSh()->varianceBuffer =
      varianceBuffer ? varianceBuffer->devicePtr() : nullptr;
  getSh()->normalBuffer = normalBuffer ? normalBuffer->devicePtr() : nullptr;
  getSh()->albedoBuffer = albedoBuffer ? albedoBuffer->devicePtr() : nullptr;
  getSh()->taskAccumID = taskAccumID ? taskAccumID->devicePtr() : nullptr;
  getSh()->taskRegionError = taskErrorRegion.errorBuffer();
  getSh()->numRenderTasks = numRenderTasks;
  getSh()->primitiveIDBuffer =
      primitiveIDBuffer ? primitiveIDBuffer->devicePtr() : nullptr;
  getSh()->objectIDBuffer =
      objectIDBuffer ? objectIDBuffer->devicePtr() : nullptr;
  getSh()->instanceIDBuffer =
      instanceIDBuffer ? instanceIDBuffer->devicePtr() : nullptr;
}

void LocalFrameBuffer::commit()
{
  FrameBuffer::commit();

  if (imageOpData) {
    FrameBufferView fbv(this,
        getColorBufferFormat(),
        getNumPixels(),
        colorBuffer ? colorBuffer->devicePtr() : nullptr,
        depthBuffer ? depthBuffer->devicePtr() : nullptr,
        normalBuffer ? normalBuffer->devicePtr() : nullptr,
        albedoBuffer ? albedoBuffer->devicePtr() : nullptr);

    prepareLiveOpsForFBV(fbv);
  }
}

vec2i LocalFrameBuffer::getNumRenderTasks() const
{
  return numRenderTasks;
}

uint32_t LocalFrameBuffer::getTotalRenderTasks() const
{
  return numRenderTasks.long_product();
}

utility::ArrayView<uint32_t> LocalFrameBuffer::getRenderTaskIDs(
    float errorThreshold)
{
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

std::string LocalFrameBuffer::toString() const
{
  return "ospray::LocalFrameBuffer";
}

void LocalFrameBuffer::clear()
{
  FrameBuffer::clear();

  // it is only necessary to reset the accumID,
  // LocalFrameBuffer_accumulateTile takes care of clearing the
  // accumulating buffers
  if (taskAccumID) {
    // TODO: Implement fill function in ISPCRT to do this through level-zero
    // on the device
    std::fill(taskAccumID->begin(), taskAccumID->end(), 0);
    device.getIspcrtQueue().copyToDevice(*taskAccumID);
  }

  // always also clear error buffer (if present)
  if (hasVarianceBuffer) {
    taskErrorRegion.clear();
  }
}
void LocalFrameBuffer::writeTiles(const utility::ArrayView<Tile> &tiles)
{
  // TODO: The parallel dispatch part of this should be moved into ISPC as an
  // ISPC launch that calls the individual (currently) exported functions that
  // we call below in this loop
#ifndef OSPRAY_TARGET_SYCL
  tasking::parallel_for(tiles.size(), [&](const size_t i) {
    const Tile *tile = &tiles[i];
    if (hasDepthBuffer) {
      ispc::LocalFrameBuffer_writeDepthTile(getSh(), tile);
    }

    if (hasAlbedoBuffer) {
      ispc::LocalFrameBuffer_writeAuxTile(getSh(),
          tile,
          (ispc::vec3f *)albedoBuffer->data(),
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

    if (hasNormalBuffer) {
      ispc::LocalFrameBuffer_writeAuxTile(getSh(),
          tile,
          (ispc::vec3f *)normalBuffer->data(),
          tile->nx,
          tile->ny,
          tile->nz);
    }
    if (colorBuffer) {
      switch (getColorBufferFormat()) {
      case OSP_FB_RGBA8: {
        ispc::LocalFrameBuffer_writeTile_RGBA8(getSh(), tile);
        break;
      }
      case OSP_FB_SRGBA: {
        ispc::LocalFrameBuffer_writeTile_SRGBA(getSh(), tile);
        break;
      }
      case OSP_FB_RGBA32F: {
        ispc::LocalFrameBuffer_writeTile_RGBA32F(getSh(), tile);
        break;
      }
      default:
        NOT_IMPLEMENTED;
      }
    }
  });

#else
  auto *fbSh = getSh();
  const size_t numTasks = tiles.size();
  const Tile *tilesPtr = tiles.data();
  const int colorFormat = getColorBufferFormat();
  vec3f *albedoBufferPtr = fbSh->super.channels & OSP_FB_ALBEDO ? albedoBuffer->data() : nullptr;
  vec3f *normalBufferPtr = fbSh->super.channels & OSP_FB_NORMAL ? normalBuffer->data() : nullptr;

  device.getSyclQueue()
      .submit([&](sycl::handler &cgh) {
        const sycl::nd_range<1> dispatchRange =
            device.computeDispatchRange(numTasks, 16);
        cgh.parallel_for(dispatchRange, [=](sycl::nd_item<1> taskIndex) {
          if (taskIndex.get_global_id(0) < numTasks) {
            const Tile *tile = &tilesPtr[taskIndex.get_global_id(0)];
            if (fbSh->super.channels & OSP_FB_DEPTH) {
              ispc::LocalFrameBuffer_writeDepthTile(fbSh, tile);
            }
            if (fbSh->super.channels & OSP_FB_ALBEDO) {
              ispc::LocalFrameBuffer_writeAuxTile(
                  fbSh, tile, albedoBufferPtr, tile->ar, tile->ag, tile->ab);
            }
            if (fbSh->super.channels & OSP_FB_ID_PRIMITIVE) {
              ispc::LocalFrameBuffer_writeIDTile(
                  fbSh, tile, fbSh->primitiveIDBuffer, tile->pid);
            }
            if (fbSh->super.channels & OSP_FB_ID_OBJECT) {
              ispc::LocalFrameBuffer_writeIDTile(
                  fbSh, tile, fbSh->objectIDBuffer, tile->gid);
            }
            if (fbSh->super.channels & OSP_FB_ID_INSTANCE) {
              ispc::LocalFrameBuffer_writeIDTile(
                  fbSh, tile, fbSh->instanceIDBuffer, tile->iid);
            }
            if (fbSh->super.channels & OSP_FB_NORMAL) {
              ispc::LocalFrameBuffer_writeAuxTile(
                  fbSh, tile, normalBufferPtr, tile->nx, tile->ny, tile->nz);
            }
            switch (colorFormat) {
            case OSP_FB_RGBA8:
              ispc::LocalFrameBuffer_writeTile_RGBA8(fbSh, tile);
              break;
            case OSP_FB_SRGBA:
              ispc::LocalFrameBuffer_writeTile_SRGBA(fbSh, tile);
              break;
            case OSP_FB_RGBA32F:
              ispc::LocalFrameBuffer_writeTile_RGBA32F(fbSh, tile);
              break;
            default:
              break;
            }
          }
        });
      })
      .wait_and_throw();

#endif
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
}

void LocalFrameBuffer::endFrame(
    const float errorThreshold, const Camera *)
{
  frameVariance = taskErrorRegion.refine(errorThreshold);
}

AsyncEvent LocalFrameBuffer::postProcess(const Camera *camera, bool wait)
{
  AsyncEvent event;
  for (auto &p : frameOps)
    p->process((wait) ? nullptr : &event, camera);
  return event;
}

const void *LocalFrameBuffer::mapBuffer(OSPFrameBufferChannel channel)
{
  const void *buf = nullptr;
  ispcrt::TaskQueue &tq = device.getIspcrtQueue();

  switch (channel) {
  case OSP_FB_COLOR: {
    tq.copyToHost(*colorBuffer);
    tq.sync();
    buf = colorBuffer ? colorBuffer->data() : nullptr;
  } break;
  case OSP_FB_DEPTH: {
    tq.copyToHost(*depthBuffer);
    tq.sync();
    buf = depthBuffer ? depthBuffer->data() : nullptr;
  } break;
  case OSP_FB_NORMAL: {
    tq.copyToHost(*normalBuffer);
    tq.sync();
    buf = normalBuffer ? normalBuffer->data() : nullptr;
  } break;
  case OSP_FB_ALBEDO: {
    tq.copyToHost(*albedoBuffer);
    tq.sync();
    buf = albedoBuffer ? albedoBuffer->data() : nullptr;
  } break;
  case OSP_FB_ID_PRIMITIVE: {
    tq.copyToHost(*primitiveIDBuffer);
    tq.sync();
    buf = primitiveIDBuffer ? primitiveIDBuffer->data() : nullptr;
  } break;
  case OSP_FB_ID_OBJECT: {
    tq.copyToHost(*objectIDBuffer);
    tq.sync();
    buf = objectIDBuffer ? objectIDBuffer->data() : nullptr;
  } break;
  case OSP_FB_ID_INSTANCE: {
    tq.copyToHost(*instanceIDBuffer);
    tq.sync();
    buf = instanceIDBuffer ? instanceIDBuffer->data() : nullptr;
  } break;
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
