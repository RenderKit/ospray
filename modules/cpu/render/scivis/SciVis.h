// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "render/Renderer.h"
// ispc shared
#include "SciVisShared.h"

namespace ospray {

struct SciVis : public AddStructShared<Renderer, ispc::SciVis>
{
  SciVis(api::ISPCDevice &device);
  std::string toString() const override;
  void commit() override;
  void *beginFrame(FrameBuffer *, World *) override;

#ifdef OSPRAY_TARGET_DPCPP
  void renderTasks(FrameBuffer *fb,
      Camera *camera,
      World *world,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs,
      sycl::queue &syclQueue) const override;

  // virtual void setGPUFunctionPtrs(sycl::queue &syclQueue) override;
#endif
};

} // namespace ospray
