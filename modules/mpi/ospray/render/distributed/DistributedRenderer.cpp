// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedRenderer.h"
#include "../../common/DistributedWorld.h"
#include "common/Instance.h"
#include "geometry/GeometricModel.h"
// ispc exports
#ifndef OSPRAY_TARGET_SYCL
#include "render/distributed/DistributedRenderer_ispc.h"
#else
namespace ispc {
SYCL_EXTERNAL void DR_default_computeRegionVisibility(Renderer *uniform self,
    SparseFB *uniform fb,
    Camera *uniform camera,
    DistributedWorld *uniform world,
    uint8 *uniform regionVisible,
    void *uniform perFrameData,
    const uint32 *uniform taskIDs,
    const int taskIndex0);
}
#endif

namespace ospray {
namespace mpi {

DistributedRenderer::DistributedRenderer(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtDevice(), device),
      mpiGroup(mpicommon::worker.dup())
{}

DistributedRenderer::~DistributedRenderer()
{
  MPI_Comm_free(&mpiGroup.comm);
}

void DistributedRenderer::computeRegionVisibility(SparseFrameBuffer *fb,
    Camera *camera,
    DistributedWorld *world,
    uint8_t *regionVisible,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs
#ifdef OSPRAY_TARGET_SYCL
    ,
    sycl::queue &syclQueue
#endif
) const
{
#ifndef OSPRAY_TARGET_SYCL
  ispc::DistributedRenderer_computeRegionVisibility(getSh(),
      fb->getSh(),
      camera->getSh(),
      world->getSh(),
      regionVisible,
      perFrameData,
      taskIDs.data(),
      taskIDs.size());
#else
  auto *rendererSh = getSh();
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const uint32_t *taskIDsPtr = taskIDs.data();
  const size_t numTasks = taskIDs.size();

  auto event = syclQueue.submit([&](sycl::handler &cgh) {
    const sycl::nd_range<1> dispatchRange = computeDispatchRange(numTasks, 16);
    cgh.parallel_for(dispatchRange, [=](sycl::nd_item<1> taskIndex) {
      if (taskIndex.get_global_id(0) < numTasks) {
        ispc::DR_default_computeRegionVisibility(rendererSh,
            fbSh,
            cameraSh,
            worldSh,
            regionVisible,
            perFrameData,
            taskIDsPtr,
            taskIndex.get_global_id(0));
      }
    });
  });
  event.wait_and_throw();
  // For prints we have to flush the entire queue, because other stuff is queued
  syclQueue.wait_and_throw();
#endif
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

#ifndef OSPRAY_TARGET_SYCL
  // TODO for SYCL need to dispatch a kernel
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
#endif

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
