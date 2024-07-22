// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <fstream>
#include <limits>
#include <map>
#include <utility>
#include "common/Data.h"
#include "lights/AmbientLight.h"
#include "lights/Light.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/utility/getEnvVar.h"

#include "AlphaCompositeTileOperation.h"
#include "DistributedRaycast.h"
#include "common/DistributedWorld.h"
#include "fb/DistributedFrameBuffer.h"

// ispc exports
#ifndef OSPRAY_TARGET_SYCL
#include "render/distributed/DistributedRaycast_ispc.h"
#else
#include "common/FeatureFlags.ih"
namespace ispc {
SYCL_EXTERNAL void DistributedRaycast_renderRegionToTileTask(void *_self,
    void *_fb,
    void *_camera,
    void *_world,
    const void *_region,
    void *perFrameData,
    const void *_taskIDs,
    const int taskIndex0,
    const uniform FeatureFlagsHandler &ffh);
}
#endif

namespace ospray {
namespace mpi {
using namespace std::chrono;
using namespace mpicommon;

// DistributedRaycastRenderer definitions /////////////////////////////////

DistributedRaycastRenderer::DistributedRaycastRenderer(api::ISPCDevice &device)
    : AddStructShared(device.getDRTDevice(), device),
      mpiGroup(mpicommon::worker.dup())
{}

DistributedRaycastRenderer::~DistributedRaycastRenderer()
{
  MPI_Comm_free(&mpiGroup.comm);
}

void DistributedRaycastRenderer::commit()
{
  Renderer::commit();

  getSh()->aoSamples = getParam<int>("aoSamples", 0);
  getSh()->aoRadius = getParam<float>("aoDistance", 1e20f);
  getSh()->shadowsEnabled = getParam<bool>("shadows", false);
  getSh()->volumeSamplingRate = getParam<float>("volumeSamplingRate", 1.f);
}

std::shared_ptr<TileOperation> DistributedRaycastRenderer::tileOperation()
{
  return std::make_shared<AlphaCompositeTileOperation>();
}

#ifndef OSPRAY_TARGET_SYCL
void DistributedRaycastRenderer::renderRegionTasks(SparseFrameBuffer *fb,
    Camera *camera,
    DistributedWorld *world,
    const box3f &region,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  ispc::DistributedRaycast_renderRegionToTileTask(getSh(),
      fb->getSh(),
      camera->getSh(),
      world->getSh(),
      (ispc::box3f *)&region,
      perFrameData,
      taskIDs.data(),
      taskIDs.size());
}
#else
void DistributedRaycastRenderer::renderRegionTasks(SparseFrameBuffer *fb,
    Camera *camera,
    DistributedWorld *world,
    const box3f &region,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  auto *rendererSh = getSh();
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const uint32_t *taskIDsPtr = taskIDs.data();
  const size_t numTasks = taskIDs.size();

  sycl::queue *queue =
      static_cast<sycl::queue *>(device.getDRTDevice().getSyclQueuePtr());
  auto event = queue->submit([&](sycl::handler &cgh) {
    FeatureFlags ff = world->getFeatureFlags();
    ff |= featureFlags;
    ff |= fb->getFeatureFlags();
    ff |= camera->getFeatureFlags();
    cgh.set_specialization_constant<ispc::specFeatureFlags>(ff);

    cgh.parallel_for(fb->getDispatchRange(numTasks),
        [=](sycl::nd_item<3> taskIndex, sycl::kernel_handler kh) {
          const box3f regionCopy = region;
          if (taskIndex.get_global_id(0) < numTasks) {
            ispc::FeatureFlagsHandler ffh(kh);
            ispc::DistributedRaycast_renderRegionToTileTask(&rendererSh->super,
                fbSh,
                cameraSh,
                worldSh,
                (ispc::box3f *)&regionCopy,
                perFrameData,
                taskIDsPtr,
                taskIndex.get_global_id(0),
                ffh);
          }
        });
  });
  event.wait_and_throw();
}
#endif

std::string DistributedRaycastRenderer::toString() const
{
  return "ospray::mpi::DistributedRaycastRenderer";
}

} // namespace mpi
} // namespace ospray
