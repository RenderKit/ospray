// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LocalFB.h"
#include <iterator>
#include "ImageOp.h"
// ispc exports
#include "fb/LocalFB_ispc.h"

namespace ospray {

template <typename T, typename A>
static T *getDataSafe(std::vector<T, A> &v)
{
  return v.empty() ? nullptr : v.data();
}

LocalFrameBuffer::LocalFrameBuffer(const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels)
    : AddStructShared(_size, _colorBufferFormat, channels),
      tileErrorRegion(hasVarianceBuffer ? getNumTiles() : vec2i(0))
{
  const size_t pixelBytes = sizeOf(_colorBufferFormat);
  const size_t numPixels = _size.long_product();

  colorBuffer.resize(pixelBytes * numPixels);

  if (hasDepthBuffer)
    depthBuffer.resize(numPixels);

  if (hasAccumBuffer)
    accumBuffer.resize(numPixels);

  tileAccumID.resize(getTotalTiles(), 0);

  if (hasVarianceBuffer)
    varianceBuffer.resize(numPixels);

  if (hasNormalBuffer)
    normalBuffer.resize(numPixels);

  if (hasAlbedoBuffer)
    albedoBuffer.resize(numPixels);

  getSh()->colorBuffer = getDataSafe(colorBuffer);
  getSh()->depthBuffer = getDataSafe(depthBuffer);
  getSh()->accumBuffer = getDataSafe(accumBuffer);
  getSh()->varianceBuffer = getDataSafe(varianceBuffer);
  getSh()->normalBuffer = getDataSafe(normalBuffer);
  getSh()->albedoBuffer = getDataSafe(albedoBuffer);
  getSh()->tileAccumID = getDataSafe(tileAccumID);
  getSh()->numTiles = getNumTiles();
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

    findFirstFrameOperation();
  }
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
  std::fill(tileAccumID.begin(), tileAccumID.end(), 0);

  // always also clear error buffer (if present)
  if (hasVarianceBuffer) {
    tileErrorRegion.clear();
  }
}

void LocalFrameBuffer::setTile(Tile &tile)
{
  if (hasAccumBuffer) {
    const float err =
        ispc::LocalFrameBuffer_accumulateTile(getSh(), (ispc::Tile &)tile);
    if ((tile.accumID & 1) == 1)
      tileErrorRegion.update(tile.region.lower / TILE_SIZE, err);
  }

  if (hasDepthBuffer) {
    ispc::LocalFrameBuffer_accumulateWriteDepthTile(
        getSh(), (ispc::Tile &)tile);
  }

  if (hasAlbedoBuffer) {
    ispc::LocalFrameBuffer_accumulateAuxTile(getSh(),
        (ispc::Tile &)tile,
        (ispc::vec3f *)albedoBuffer.data(),
        tile.ar,
        tile.ag,
        tile.ab);
  }

  if (hasNormalBuffer)
    ispc::LocalFrameBuffer_accumulateAuxTile(getSh(),
        (ispc::Tile &)tile,
        (ispc::vec3f *)normalBuffer.data(),
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

  if (!colorBuffer.empty()) {
    switch (getColorBufferFormat()) {
    case OSP_FB_RGBA8:
      ispc::LocalFrameBuffer_writeTile_RGBA8(getSh(), (ispc::Tile &)tile);
      break;
    case OSP_FB_SRGBA:
      ispc::LocalFrameBuffer_writeTile_SRGBA(getSh(), (ispc::Tile &)tile);
      break;
    case OSP_FB_RGBA32F:
      ispc::LocalFrameBuffer_writeTile_RGBA32F(getSh(), (ispc::Tile &)tile);
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
