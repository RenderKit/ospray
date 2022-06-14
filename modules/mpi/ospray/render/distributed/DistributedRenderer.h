// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../fb/DistributedFrameBuffer.h"
#include "../../fb/TileOperation.h"
#include "camera/Camera.h"
#include "render/Renderer.h"
// ispc shared
#include "DistributedRendererShared.h"

namespace ospray {
namespace mpi {

struct DistributedWorld;

struct RegionInfo
{
  int numRegions = 0;
  box3f *regions = nullptr;
  uint8_t *regionVisible = nullptr;
};

struct DistributedRenderer
    : public AddStructShared<Renderer, ispc::DistributedRenderer>
{
  DistributedRenderer();
  ~DistributedRenderer() override;

  void computeRegionVisibility(SparseFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      uint8_t *regionVisible,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs) const;

  void renderRegionTasks(SparseFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      const box3f &region,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs) const;

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
