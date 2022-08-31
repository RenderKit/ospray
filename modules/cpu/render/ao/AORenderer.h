// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "render/Renderer.h"
// ispc shared
#include "AORendererShared.h"

namespace ospray {

struct AORenderer : public AddStructShared<Renderer, ispc::AORenderer>
{
  AORenderer(api::ISPCDevice &device, int defaultAORendererSamples = 1);
  std::string toString() const override;
  void commit() override;

#ifdef OSPRAY_TARGET_SYCL
  void renderTasks(FrameBuffer *fb,
      Camera *camera,
      World *world,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs,
      sycl::queue &syclQueue) const override;
#endif

 private:
  int aoSamples{1};
};

} // namespace ospray
