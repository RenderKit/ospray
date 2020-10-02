// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedRenderer.h"
#include "render/distributed/DistributedRenderer_ispc.h"

namespace ospray {
namespace mpi {

void DistributedRenderer::computeRegionVisibility(DistributedFrameBuffer *fb,
    Camera *camera,
    DistributedWorld *world,
    bool *regionVisible,
    void *perFrameData,
    Tile &tile,
    size_t jobID) const
{
  // TODO this needs an exported function
  ispc::DistributedRenderer_computeRegionVisibility(getIE(),
      fb->getIE(),
      camera->getIE(),
      world->getIE(),
      regionVisible,
      perFrameData,
      (ispc::Tile &)tile,
      jobID);
}

void DistributedRenderer::renderRegionToTile(DistributedFrameBuffer *fb,
    Camera *camera,
    DistributedWorld *world,
    const Region &region,
    void *perFrameData,
    Tile &tile,
    size_t jobID) const
{
  // TODO: exported fcn
  ispc::DistributedRenderer_renderRegionToTile(getIE(),
      fb->getIE(),
      camera->getIE(),
      world->getIE(),
      &region,
      perFrameData,
      (ispc::Tile &)tile,
      jobID);
}

} // namespace mpi
} // namespace ospray
