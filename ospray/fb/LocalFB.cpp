// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LocalFB.h"
#include <iterator>
#include "ImageOp.h"
#include "LocalFB_ispc.h"

namespace ospray {

LocalFrameBuffer::LocalFrameBuffer(const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels,
    void *colorBufferToUse)
    : FrameBuffer(_size, _colorBufferFormat, channels),
      tileErrorRegion(hasVarianceBuffer ? getNumTiles() : vec2i(0))
{
  if (size.x <= 0 || size.y <= 0) {
    throw std::runtime_error(
        "local framebuffer has invalid size. Dimensions must be greater than "
        "0");
  }

  if (colorBufferToUse)
    colorBuffer = colorBufferToUse;
  else {
    switch (colorBufferFormat) {
    case OSP_FB_NONE:
      colorBuffer = nullptr;
      break;
    case OSP_FB_RGBA8:
    case OSP_FB_SRGBA:
      colorBuffer = (uint32 *)alignedMalloc(sizeof(uint32) * size.x * size.y);
      break;
    case OSP_FB_RGBA32F:
      colorBuffer = (vec4f *)alignedMalloc(sizeof(vec4f) * size.x * size.y);
      break;
    }
  }

  depthBuffer =
      hasDepthBuffer ? alignedMalloc<float>(size.x * size.y) : nullptr;

  accumBuffer =
      hasAccumBuffer ? alignedMalloc<vec4f>(size.x * size.y) : nullptr;

  const size_t bytes = sizeof(int32) * getTotalTiles();
  tileAccumID = (int32 *)alignedMalloc(bytes);
  memset(tileAccumID, 0, bytes);

  varianceBuffer =
      hasVarianceBuffer ? alignedMalloc<vec4f>(size.x * size.y) : nullptr;

  normalBuffer =
      hasNormalBuffer ? alignedMalloc<vec3f>(size.x * size.y) : nullptr;

  albedoBuffer =
      hasAlbedoBuffer ? alignedMalloc<vec3f>(size.x * size.y) : nullptr;

  ispcEquivalent = ispc::LocalFrameBuffer_create(this,
      size.x,
      size.y,
      colorBufferFormat,
      colorBuffer,
      depthBuffer,
      accumBuffer,
      varianceBuffer,
      normalBuffer,
      albedoBuffer,
      tileAccumID);
}

LocalFrameBuffer::~LocalFrameBuffer()
{
  alignedFree(depthBuffer);
  alignedFree(colorBuffer);
  alignedFree(accumBuffer);
  alignedFree(varianceBuffer);
  alignedFree(normalBuffer);
  alignedFree(albedoBuffer);
  alignedFree(tileAccumID);
}

void LocalFrameBuffer::commit()
{
  FrameBuffer::commit();

  imageOps.clear();
  if (imageOpData) {
    FrameBufferView fbv(this,
        colorBufferFormat,
        colorBuffer,
        depthBuffer,
        normalBuffer,
        albedoBuffer);

    for (auto &&obj : *imageOpData)
      imageOps.push_back(obj->attach(fbv));

    findFirstFrameOperation();
  }
}

std::string LocalFrameBuffer::toString() const
{
  return "ospray::LocalFrameBuffer";
}

void LocalFrameBuffer::clear()
{
  frameID = -1; // we increment at the start of the frame
  // it is only necessary to reset the accumID,
  // LocalFrameBuffer_accumulateTile takes care of clearing the
  // accumulating buffers
  memset(tileAccumID, 0, getTotalTiles() * sizeof(int32));

  // always also clear error buffer (if present)
  if (hasVarianceBuffer) {
    tileErrorRegion.clear();
  }
}

void LocalFrameBuffer::setTile(Tile &tile)
{
  if (accumBuffer) {
    const float err =
        ispc::LocalFrameBuffer_accumulateTile(getIE(), (ispc::Tile &)tile);
    if ((tile.accumID & 1) == 1)
      tileErrorRegion.update(tile.region.lower / TILE_SIZE, err);
  }
  if (hasAlbedoBuffer)
    ispc::LocalFrameBuffer_accumulateAuxTile(getIE(),
        (ispc::Tile &)tile,
        (ispc::vec3f *)albedoBuffer,
        tile.ar,
        tile.ag,
        tile.ab);
  if (hasNormalBuffer)
    ispc::LocalFrameBuffer_accumulateAuxTile(getIE(),
        (ispc::Tile &)tile,
        (ispc::vec3f *)normalBuffer,
        tile.nx,
        tile.ny,
        tile.nz);

  if (!imageOps.empty()) {
    std::for_each(imageOps.begin(),
        imageOps.begin() + firstFrameOperation,
        [&](std::unique_ptr<LiveImageOp> &iop) {
          LiveTileOp *top = dynamic_cast<LiveTileOp *>(iop.get());
          if (top) {
            top->process(tile);
          }
          // TODO: For now, frame operations must be last
          // in the pipeline
        });
  }

  if (colorBuffer) {
    switch (colorBufferFormat) {
    case OSP_FB_RGBA8:
      ispc::LocalFrameBuffer_writeTile_RGBA8(getIE(), (ispc::Tile &)tile);
      break;
    case OSP_FB_SRGBA:
      ispc::LocalFrameBuffer_writeTile_SRGBA(getIE(), (ispc::Tile &)tile);
      break;
    case OSP_FB_RGBA32F:
      ispc::LocalFrameBuffer_writeTile_RGBA32F(getIE(), (ispc::Tile &)tile);
      break;
    default:
      NOT_IMPLEMENTED;
    }
  }
}

int32 LocalFrameBuffer::accumID(const vec2i &tile)
{
  return tileAccumID[tile.y * numTiles.x + tile.x];
}

float LocalFrameBuffer::tileError(const vec2i &tile)
{
  return tileErrorRegion[tile];
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

  frameVariance = tileErrorRegion.refine(errorThreshold);
}

const void *LocalFrameBuffer::mapBuffer(OSPFrameBufferChannel channel)
{
  const void *buf;
  switch (channel) {
  case OSP_FB_COLOR:
    buf = colorBuffer;
    break;
  case OSP_FB_DEPTH:
    buf = depthBuffer;
    break;
  case OSP_FB_NORMAL:
    buf = normalBuffer;
    break;
  case OSP_FB_ALBEDO:
    buf = albedoBuffer;
    break;
  default:
    buf = nullptr;
    break;
  }

  if (buf)
    this->refInc();

  return buf;
}

void LocalFrameBuffer::unmap(const void *mappedMem)
{
  if (mappedMem) {
    if (mappedMem != colorBuffer && mappedMem != depthBuffer
        && mappedMem != normalBuffer && mappedMem != albedoBuffer) {
      throw std::runtime_error("unmapping a pointer not created by OSPRay");
    }
    this->refDec();
  }
}

} // namespace ospray
