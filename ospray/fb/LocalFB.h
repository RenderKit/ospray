// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "fb/FrameBuffer.h"
#include "fb/TileError.h"

namespace ospray {

/*! local frame buffer - frame buffer that exists on local machine */
struct OSPRAY_SDK_INTERFACE LocalFrameBuffer : public FrameBuffer
{
  // TODO: Replace w/ some std::vector
  void *colorBuffer; /*!< format depends on
                        FrameBuffer::colorBufferFormat, may be
                        NULL */
  float *depthBuffer; /*!< one float per pixel, may be NULL */
  vec4f *accumBuffer; /*!< one RGBA per pixel, may be NULL */
  vec4f *varianceBuffer; /*!< one RGBA per pixel, may be NULL, accumulates every
                            other sample, for variance estimation / stopping */
  vec3f *normalBuffer; /*!< accumulated world-space normal per pixel, may be
                          NULL */
  vec3f *albedoBuffer; /*!< accumulated, one RGB per pixel, may be NULL */
  int32 *tileAccumID; //< holds accumID per tile, for adaptive accumulation
  TileError tileErrorRegion; /*!< holds error per tile and adaptive regions, for
                                variance estimation / stopping */

  LocalFrameBuffer(const vec2i &size,
      ColorBufferFormat colorBufferFormat,
      const uint32 channels,
      void *colorBufferToUse = nullptr);
  virtual ~LocalFrameBuffer() override;

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
};

} // namespace ospray
