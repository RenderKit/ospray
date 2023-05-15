// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "camera/Camera.h"
#include "fb/DistributedFrameBuffer.h"
#include "fb/TileOperation.h"
#include "render/Renderer.h"
// ispc shared
#include "render/RendererShared.h"

namespace ospray {
namespace mpi {

struct DistributedWorld;

struct RegionInfo
{
  int numRegions = 0;
  box3f *regions = nullptr;
  uint8_t *regionVisible = nullptr;
};

struct DistributedRenderer : public AddStructShared<Renderer, ispc::Renderer>
{
  DistributedRenderer(api::ISPCDevice &device);
  ~DistributedRenderer() override;

  void computeRegionVisibility(SparseFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      uint8_t *regionVisible,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs) const;

  // Not used by distributed renderers
  AsyncEvent renderTasks(FrameBuffer *,
      Camera *,
      World *,
      void * /*perFrameData*/,
      const utility::ArrayView<uint32_t> & /*taskIDs*/,
      bool /*wait*/) const override
  {
    return AsyncEvent();
  }

#ifndef OSPRAY_TARGET_SYCL
  virtual void renderRegionTasks(SparseFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      const box3f &region,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs) const = 0;
#else
  virtual void renderRegionTasks(SparseFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      const box3f &region,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs) const = 0;
#endif

  virtual std::shared_ptr<TileOperation> tileOperation() = 0;

  // Picking in the distributed renderer needs to be clipped to the regions
  // specified to bound the local data
  OSPPickResult pick(FrameBuffer *fb,
      Camera *camera,
      World *world,
      const vec2f &screenPos) override;

 private:
  mpicommon::Group mpiGroup;
};
} // namespace mpi
} // namespace ospray
