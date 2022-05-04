// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedRenderer.h"
#include "../../common/DistributedWorld.h"
#include "common/Instance.h"
#include "geometry/GeometricModel.h"
// ispc exports
#include "render/distributed/DistributedRenderer_ispc.h"

namespace ospray {
namespace mpi {

DistributedRenderer::DistributedRenderer() : mpiGroup(mpicommon::worker.dup())
{
  getSh()->computeRegionVisibility =
      ispc::DR_default_computeRegionVisibility_addr();
  getSh()->renderRegionSample = ispc::DR_default_renderRegionSample_addr();
  getSh()->renderRegionToTile = ispc::DR_default_renderRegionToTile_addr();
}

DistributedRenderer::~DistributedRenderer()
{
  MPI_Comm_free(&mpiGroup.comm);
}

void DistributedRenderer::computeRegionVisibility(SparseFrameBuffer *fb,
    Camera *camera,
    DistributedWorld *world,
    uint8_t *regionVisible,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  // TODO this needs an exported function
  ispc::DistributedRenderer_computeRegionVisibility(getSh(),
      fb->getSh(),
      camera->getSh(),
      world->getSh(),
      regionVisible,
      perFrameData,
      taskIDs.data(),
      taskIDs.size());
}

void DistributedRenderer::renderRegionTasks(SparseFrameBuffer *fb,
    Camera *camera,
    DistributedWorld *world,
    const box3f &region,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  // TODO: exported fcn
  ispc::DistributedRenderer_renderRegionToTile(getSh(),
      fb->getSh(),
      camera->getSh(),
      world->getSh(),
      &region,
      perFrameData,
      taskIDs.data(),
      taskIDs.size());
}

OSPPickResult DistributedRenderer::pick(
    FrameBuffer *fb, Camera *camera, World *world, const vec2f &screenPos)
{
  OSPPickResult res;

  res.instance = nullptr;
  res.model = nullptr;
  res.primID = RTC_INVALID_GEOMETRY_ID;
  res.hasHit = false;

  int instID = RTC_INVALID_GEOMETRY_ID;
  int geomID = RTC_INVALID_GEOMETRY_ID;
  int primID = RTC_INVALID_GEOMETRY_ID;
  float depth = 1e20f;

  ispc::DistributedRenderer_pick(getSh(),
      fb->getSh(),
      camera->getSh(),
      world->getSh(),
      (const ispc::vec2f &)screenPos,
      (ispc::vec3f &)res.worldPosition[0],
      instID,
      geomID,
      primID,
      depth,
      res.hasHit);

  // Find the closest picked object globally, only the rank
  // with this object will report the pick
  float globalDepth = 1e20f;
  mpicommon::allreduce(
      &depth, &globalDepth, 1, MPI_FLOAT, MPI_MIN, mpiGroup.comm)
      .wait();

  res.hasHit = globalDepth < 1e20f && globalDepth == depth;

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
