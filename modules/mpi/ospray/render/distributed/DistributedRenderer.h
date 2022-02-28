// Copyright 2009-2022 Intel Corporation
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
  bool *regionVisible = nullptr;
};

struct DistributedRenderer
    : public AddStructShared<Renderer, ispc::DistributedRenderer>
{
  DistributedRenderer();
  ~DistributedRenderer() override;

  void computeRegionVisibility(DistributedFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      bool *regionVisible,
      void *perFrameData,
      Tile &tile,
      size_t jobID) const;

  void renderRegionToTile(DistributedFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      const box3f &region,
      void *perFrameData,
      Tile &tile,
      size_t jobID) const;

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
