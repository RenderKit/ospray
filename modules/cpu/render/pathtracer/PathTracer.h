// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/Renderer.h"
// ispc shared
#include "PathTracerShared.h"

namespace ispc {
struct Light;
}

namespace ospray {

struct PathTracer : public AddStructShared<Renderer, ispc::PathTracer>
{
  PathTracer(api::ISPCDevice &device);
  virtual std::string toString() const override;
  virtual void commit() override;
  virtual void *beginFrame(FrameBuffer *, World *) override;

  virtual AsyncEvent renderTasks(FrameBuffer *fb,
      Camera *camera,
      World *world,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs,
      bool wait) const override;

 private:
  bool importanceSampleGeometryLights{
      true}; // if geometry lights are importance
             // sampled using NEE (requires scanning
             // the scene for geometric lights)
  bool scannedGeometryLights{
      false}; // if the scene was scanned for geometric lights
};

} // namespace ospray
