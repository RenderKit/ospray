// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LocalFB.h"
#include "FrameOp.h"
#include "SparseFB.h"
#include "fb/FrameBufferView.h"
#include "frame_ops/ColorConversion.h"
#include "frame_ops/Variance.h"
#include "render/util.h"
#include "rkcommon/common.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/utility/ArrayView.h"

#ifndef OSPRAY_TARGET_SYCL
#include "fb/LocalFB_ispc.h"
#else
namespace ispc {

SYCL_EXTERNAL void LocalFrameBuffer_writeColorTile(
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
    : AddStructShared(device.getDRTDevice(),
        device,
        _size,
        _colorBufferFormat,
        channels,
        FFO_FB_LOCAL),
      device(device),
      numRenderTasks(divRoundUp(size, getRenderTaskSize()))
{
  const size_t numPixels = _size.long_product();
  if (hasColorBuffer)
    colorBuffer = devicert::make_buffer_device_shadowed_unique<vec4f>(
        device.getDRTDevice(), numPixels);

  if (hasDepthBuffer)
    depthBuffer = devicert::make_buffer_device_shadowed_unique<float>(
        device.getDRTDevice(), numPixels);

  // If variance is going to be used we need to construct helper buffer and
  // FrameOp to do the calculations
  if (hasVarianceBuffer) {
    varianceBuffer = devicert::make_buffer_device_unique<vec4f>(
        device.getDRTDevice(), numPixels);

    // Create and initialize FrameBufferView structure which define domain for
    // VarianceFrameOp. Since the VarianceFrameOp calculates per-RenderTask
    // error, the viewDims member is RenderTasks dimensions rather then
    // FrameBuffer dimensions. Perhaps FrameBufferView should be renamed to
    // FrameOpDomain or similar in future.
    FrameBufferView fbv(getNumPixels(), colorBuffer->devicePtr());
    fbv.viewDims = getNumRenderTasks();
    varianceFrameOp = rkcommon::make_unique<LiveVarianceFrameOp>(
        device.getDRTDevice(), fbv, varianceBuffer->devicePtr());
  }

  if (hasNormalBuffer)
    normalBuffer = devicert::make_buffer_device_shadowed_unique<vec3f>(
        device.getDRTDevice(), numPixels);

  if (hasAlbedoBuffer)
    albedoBuffer = devicert::make_buffer_device_shadowed_unique<vec3f>(
        device.getDRTDevice(), numPixels);

  if (hasPrimitiveIDBuffer)
    primitiveIDBuffer = devicert::make_buffer_device_shadowed_unique<uint32_t>(
        device.getDRTDevice(), numPixels);

  if (hasObjectIDBuffer)
    objectIDBuffer = devicert::make_buffer_device_shadowed_unique<uint32_t>(
        device.getDRTDevice(), numPixels);

  if (hasInstanceIDBuffer)
    instanceIDBuffer = devicert::make_buffer_device_shadowed_unique<uint32_t>(
        device.getDRTDevice(), numPixels);

  // Create color conversion FrameOp if needed
  if ((hasColorBuffer) && (getColorBufferFormat() != OSP_FB_RGBA32F)) {
    FrameBufferView fbv(getNumPixels(), colorBuffer->devicePtr());
    colorConversionFrameOp = rkcommon::make_unique<LiveColorConversionFrameOp>(
        device.getDRTDevice(), fbv, getColorBufferFormat());
  }

  // TODO: Better way to pass the task IDs that doesn't require just storing
  // them all? Maybe as blocks/tiles similar to when we just had tiles? Will
  // make task ID lookup more expensive for sparse case though
  renderTaskIDs = devicert::make_buffer_device_shadowed_unique<uint32_t>(
      device.getDRTDevice(), getTotalRenderTasks());
  std::iota(renderTaskIDs->begin(), renderTaskIDs->end(), 0);
  if (hasVarianceBuffer)
    activeTaskIDs = devicert::make_buffer_device_shadowed_unique<uint32_t>(
        device.getDRTDevice(), getTotalRenderTasks());

    // TODO: Could use TBB parallel sort here if it's exposed through the
    // rkcommon tasking system
#ifndef OSPRAY_TARGET_SYCL
  // We use a 1x1 task size in SYCL and this sorting may not pay off for the
  // cost it adds
  std::sort(renderTaskIDs->begin(),
      renderTaskIDs->end(),
      [&](const uint32_t &a, const uint32_t &b) {
        const vec2i p_a = getTaskStartPos(a);
        const vec2i p_b = getTaskStartPos(b);
        return interleaveZOrder(p_a.x, p_a.y) < interleaveZOrder(p_b.x, p_b.y);
      });
#endif
  {
    // Upload the task IDs to the device
    renderTaskIDs->copyToDevice();
  }

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
  getSh()->varianceBuffer =
      varianceBuffer ? varianceBuffer->devicePtr() : nullptr;
  getSh()->depthBuffer = depthBuffer ? depthBuffer->devicePtr() : nullptr;
  getSh()->normalBuffer = normalBuffer ? normalBuffer->devicePtr() : nullptr;
  getSh()->albedoBuffer = albedoBuffer ? albedoBuffer->devicePtr() : nullptr;
  getSh()->numRenderTasks = numRenderTasks;
  getSh()->primitiveIDBuffer =
      primitiveIDBuffer ? primitiveIDBuffer->devicePtr() : nullptr;
  getSh()->objectIDBuffer =
      objectIDBuffer ? objectIDBuffer->devicePtr() : nullptr;
  getSh()->instanceIDBuffer =
      instanceIDBuffer ? instanceIDBuffer->devicePtr() : nullptr;
}

// Need this destructor to be in the cpp where we have full definition of
// LiveVarianceFrameOp and LiveColorConversionFrameOp
LocalFrameBuffer::~LocalFrameBuffer() {}

void LocalFrameBuffer::commit()
{
  FrameBuffer::commit();

  // No frame operations if there is no color buffer
  if (!hasColorBuffer)
    return;

  FrameBufferView fbv(getNumPixels(),
      colorBuffer ? colorBuffer->devicePtr() : nullptr,
      depthBuffer ? depthBuffer->devicePtr() : nullptr,
      normalBuffer ? normalBuffer->devicePtr() : nullptr,
      albedoBuffer ? albedoBuffer->devicePtr() : nullptr);

  // Initialize user defined image operations
  ppColorBuffer.reset();
  frameOps.clear();
  if (imageOpData) {
    // Create buffer for post-processing output
    ppColorBuffer = devicert::make_buffer_device_shadowed_unique<vec4f>(
        device.getDRTDevice(), getNumPixels().long_product());
    fbv.colorBufferOutput = ppColorBuffer->devicePtr();

    // Build FrameOps chain by iterating through all image operations set on
    // commit
    for (auto &&obj : *imageOpData) {
      // Populate frame operations
      FrameOpInterface *fopi = dynamic_cast<FrameOpInterface *>(obj);
      if (fopi) {
        // Create live FrameOp object
        frameOps.push_back(fopi->attach(fbv));

        // Connect previous FrameOp output with the next FrameOp input
        fbv.colorBufferInput = fbv.colorBufferOutput;
      }
    }
  }

  // Create color conversion FrameOp if needed
  colorConversionFrameOp.reset();
  if (getColorBufferFormat() != OSP_FB_RGBA32F) {
    colorConversionFrameOp = rkcommon::make_unique<LiveColorConversionFrameOp>(
        device.getDRTDevice(), fbv, getColorBufferFormat());
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
    const float errorThreshold_, const uint32_t spp)
{
  if (accumulationFinished())
    return utility::ArrayView<uint32_t>();

  errorThreshold = errorThreshold_; // remember
  if (errorThreshold > 0.0f && varianceFrameOp
      && (getFrameID() >= minimumAdaptiveFrames(spp))) {
    // Select render tasks that needs to be processed
    auto last = std::copy_if(renderTaskIDs->begin(),
        renderTaskIDs->end(),
        activeTaskIDs->begin(),
        [=](uint32_t i) {
          return varianceFrameOp->getError(i) > errorThreshold;
        });

    activeTaskIDs->copyToDevice();
    const size_t numActive = last - activeTaskIDs->begin();
    return utility::ArrayView<uint32_t>(activeTaskIDs->devicePtr(), numActive);
  } else
    return utility::ArrayView<uint32_t>(
        renderTaskIDs->devicePtr(), renderTaskIDs->size());
}

float LocalFrameBuffer::getVariance() const
{
  // Return maximum error over all tasks if variance has been calculated in a
  // FrameOp
  if (varianceFrameOp && varianceFrameOp->validError())
    return varianceFrameOp->getAvgError(errorThreshold);

  // Return set value otherwise
  return FrameBuffer::getVariance();
}

std::string LocalFrameBuffer::toString() const
{
  return "ospray::LocalFrameBuffer";
}

void LocalFrameBuffer::clear()
{
  FrameBuffer::clear();

  if (hasVarianceBuffer && varianceFrameOp)
    varianceFrameOp->restart();
}
void LocalFrameBuffer::writeTiles(const utility::ArrayView<Tile> &tiles)
{
  // TODO: The parallel dispatch part of this should be moved into ISPC as an
  // ISPC launch that calls the individual (currently) exported functions that
  // we call below in this loop
#ifndef OSPRAY_TARGET_SYCL
  tasking::parallel_for(tiles.size(), [&](const size_t i) {
    const Tile *tile = &tiles[i];

    if (hasColorBuffer) {
      ispc::LocalFrameBuffer_writeColorTile(getSh(), tile);
    }

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
  });

#else
  auto *fbSh = getSh();
  const size_t numTasks = tiles.size();
  const Tile *tilesPtr = tiles.data();
  const int colorFormat = getColorBufferFormat();
  vec3f *albedoBufferPtr = fbSh->super.channels & OSP_FB_ALBEDO
      ? albedoBuffer->devicePtr()
      : nullptr;
  vec3f *normalBufferPtr = fbSh->super.channels & OSP_FB_NORMAL
      ? normalBuffer->devicePtr()
      : nullptr;
  sycl::queue *queue =
      static_cast<sycl::queue *>(device.getDRTDevice().getSyclQueuePtr());
  queue
      ->submit([&](sycl::handler &cgh) {
        const sycl::nd_range<1> dispatchRange =
            device.computeDispatchRange(numTasks, 16);
        cgh.parallel_for(dispatchRange, [=](sycl::nd_item<1> taskIndex) {
          if (taskIndex.get_global_id(0) < numTasks) {
            const Tile *tile = &tilesPtr[taskIndex.get_global_id(0)];
            if (fbSh->super.channels & OSP_FB_COLOR) {
              ispc::LocalFrameBuffer_writeColorTile(fbSh, tile);
            }
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
          }
        });
      })
      .wait_and_throw();
#endif
}

void LocalFrameBuffer::writeTiles(SparseFrameBuffer *sparseFb)
{
  // Write tiles operates on device memory
  writeTiles(sparseFb->getTilesDevice());

  assert(getRenderTaskSize() == sparseFb->getRenderTaskSize());
  const vec2i renderTaskSize = getRenderTaskSize();

  if (!hasVarianceBuffer) {
    return;
  }

  // Task error is not calculated by varianceFrameOp but is being written
  // directly from SparseFB, the varianceFrameOp needs to be removed to not
  // interfere with frameVariance calculation
  varianceFrameOp.reset();

  // Now we do need the tile memory on the host to read the region information
  frameVariance = 0.f;
  const auto tileIDs = sparseFb->getTileIDs();
  uint32_t renderTaskID = 0;
  for (size_t i = 0; i < tileIDs.size(); ++i) {
    const box2i tileRegion = sparseFb->getTileRegion(tileIDs[i]);
    const box2i taskRegion(
        tileRegion.lower / renderTaskSize, tileRegion.upper / renderTaskSize);
    for (int y = taskRegion.lower.y; y < taskRegion.upper.y; ++y) {
      for (int x = taskRegion.lower.x; x < taskRegion.upper.x;
           ++x, ++renderTaskID) {
        frameVariance = max(frameVariance, sparseFb->taskError(renderTaskID));
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

devicert::AsyncEvent LocalFrameBuffer::postProcess()
{
  // Calculate per-task variance if any samples accumulated into variance
  // buffer, skip it if frameVariance overridden in writeTiles()
  devicert::AsyncEvent event;
  const bool oddFrame = (getSh()->super.frameID & 1) == 1;
  if (varianceFrameOp && oddFrame && (frameVariance == float(inf)))
    event = varianceFrameOp->process();

  // Execute user-defined post-processing kernels
  for (auto &p : frameOps)
    event = p->process();

  // Run final color conversion if needed
  if (colorConversionFrameOp)
    event = colorConversionFrameOp->process();

  // Return asynchronous event
  return event;
}

namespace {
template <typename T>
const void *copyToHost(T &buffer)
{
  buffer.copyToHost().wait();
  return static_cast<const void *>(buffer.hostPtr());
}
} // namespace

const void *LocalFrameBuffer::mapBuffer(OSPFrameBufferChannel channel)
{
  const void *buf = nullptr;

  if (channel == OSP_FB_COLOR) {
    if (colorConversionFrameOp)
      buf = copyToHost(colorConversionFrameOp->getConvertedBuffer());
    else if (ppColorBuffer)
      buf = copyToHost(*ppColorBuffer);
    else if (colorBuffer)
      buf = copyToHost(*colorBuffer);
  } else if ((channel == OSP_FB_DEPTH) && (depthBuffer)) {
    buf = copyToHost(*depthBuffer);
  } else if ((channel == OSP_FB_NORMAL) && (normalBuffer)) {
    buf = copyToHost(*normalBuffer);
  } else if ((channel == OSP_FB_ALBEDO) && (albedoBuffer)) {
    buf = copyToHost(*albedoBuffer);
  } else if ((channel == OSP_FB_ID_PRIMITIVE) && (primitiveIDBuffer)) {
    buf = copyToHost(*primitiveIDBuffer);
  } else if ((channel == OSP_FB_ID_OBJECT) && (objectIDBuffer)) {
    buf = copyToHost(*objectIDBuffer);
  } else if ((channel == OSP_FB_ID_INSTANCE) && (instanceIDBuffer)) {
    buf = copyToHost(*instanceIDBuffer);
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
