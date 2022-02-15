// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "fb/FrameBuffer.h"
#include "fb/TileError.h"
// rkcommon
#include "rkcommon/containers/AlignedVector.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE LocalFrameBuffer : public FrameBuffer
{
  LocalFrameBuffer(const vec2i &size,
      ColorBufferFormat colorBufferFormat,
      const uint32 channels);
  virtual ~LocalFrameBuffer() override = default;

  virtual void commit() override;

  //! \brief common function to help printf-debugging
  /*! \detailed Every derived class should override this! */
  virtual std::string toString() const override;

  void setTile(Tile &tile) override;
  int32 accumID(const vec2i &tile) override;
  float tileError(const vec2i &tile) override;
  void beginFrame() override;
  void endFrame(const float errorThreshold, const Camera *camera) override;

  const void *mapBuffer(OSPFrameBufferChannel channel) override;
  void unmap(const void *mappedMem) override;
  void clear() override;

  //// Data ////

  // NOTE: All per-pixel data is only allocated if the corresponding channel
  //       flag was passed on construction

  // format depends on FrameBuffer::colorBufferFormat
  std::vector<uint8_t> colorBuffer;
  // one float per pixel
  containers::AlignedVector<float> depthBuffer;
  // one RGBA per pixel
  containers::AlignedVector<vec4f> accumBuffer;
  // one RGBA per pixel, accumulates every other sample, for variance estimation
  containers::AlignedVector<vec4f> varianceBuffer;
  // accumulated world-space normal per pixel
  containers::AlignedVector<vec3f> normalBuffer;
  // accumulated, one RGB per pixel
  containers::AlignedVector<vec3f> albedoBuffer;
  // holds accumID per tile, for adaptive accumulation
  containers::AlignedVector<int32> tileAccumID;
  // holds error per tile and adaptive regions
  TileError tileErrorRegion;
};

} // namespace ospray
