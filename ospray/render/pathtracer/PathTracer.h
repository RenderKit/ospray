// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Material.h"
#include "render/Renderer.h"

namespace ospray {

struct PathTracer : public Renderer
{
  PathTracer();
  virtual ~PathTracer() override;
  virtual std::string toString() const override;
  virtual void commit() override;
  virtual void *beginFrame(FrameBuffer *, World *) override;

  void generateGeometryLights(const World &);
  void destroyGeometryLights();

  // the 'IE's of the XXXLights
  std::vector<void *> lightArray;
  // number of GeometryLights at beginning of lightArray
  size_t geometryLights{0};
  bool useGeometryLights{true};
  OSPTexture bgTexture{nullptr};
};

} // namespace ospray
