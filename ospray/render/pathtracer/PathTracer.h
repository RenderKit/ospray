// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/Material.h"
#include "render/Renderer.h"

namespace ospray {

struct PathTracer : public Renderer
{
  PathTracer();
  virtual std::string toString() const override;
  virtual void commit() override;
  virtual void *beginFrame(FrameBuffer *, World *) override;

 private:
  void generateGeometryLights(const World &, std::vector<void *> &);
  bool useGeometryLights{true};
};

} // namespace ospray
