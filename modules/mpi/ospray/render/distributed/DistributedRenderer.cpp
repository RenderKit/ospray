// Copyright 2009-2021 Intel Corporation
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

OSPPickResult DistributedRenderer::pick(
    FrameBuffer *fb, Camera *camera, World *world, const vec2f &screenPos)
{
  OSPPickResult res;

  res.instance = nullptr;
  res.model = nullptr;
  res.primID = -1;
  res.hasHit = false;

  int instID = -1;
  int geomID = -1;
  int primID = -1;

  ispc::DistributedRenderer_pick(getIE(),
      fb->getIE(),
      camera->getIE(),
      world->getIE(),
      (const ispc::vec2f &)screenPos,
      (ispc::vec3f &)res.worldPosition[0],
      instID,
      geomID,
      primID,
      res.hasHit);

  if (res.hasHit) {
    auto *instance = (*world->instances)[instID];
    auto *group = instance->group.ptr;
    if (!group->geometricModels) {
      res.hasHit = false;
      return res;
    }
    auto *model = (*group->geometricModels)[geomID];

    instance->refInc();
    model->refInc();

    res.instance = (OSPInstance)instance;
    res.model = (OSPGeometricModel)model;
    res.primID = static_cast<uint32_t>(primID);
  }

  return res;
}

} // namespace mpi
} // namespace ospray
