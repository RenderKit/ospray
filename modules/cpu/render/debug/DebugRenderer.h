// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "DebugRendererShared.h"
#include "render/Renderer.h"

namespace ospray {

struct DebugRenderer : public AddStructShared<Renderer, ispc::DebugRenderer>
{
  DebugRenderer(api::ISPCDevice &device);
  virtual ~DebugRenderer() override = default;

  std::string toString() const override;

  void commit() override;

  virtual void renderTasks(FrameBuffer *fb,
      Camera *camera,
      World *world,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs
#ifdef OSPRAY_TARGET_SYCL
      ,
      sycl::queue &syclQueue
#endif
  ) const override;
};

} // namespace ospray
