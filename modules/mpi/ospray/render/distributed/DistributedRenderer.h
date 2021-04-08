// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../common/DistributedWorld.h"
#include "../../fb/DistributedFrameBuffer.h"
#include "../../fb/TileOperation.h"
#include "camera/Camera.h"
#include "render/Renderer.h"

namespace ospray {
namespace mpi {

struct RegionInfo
{
  int numRegions = 0;
  Region *regions = nullptr;
  bool *regionVisible = nullptr;
};

struct DistributedRenderer : public Renderer
{
  DistributedRenderer();

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
      const Region &region,
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
