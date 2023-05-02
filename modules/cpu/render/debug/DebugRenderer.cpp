// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "DebugRenderer.h"
#include "camera/Camera.h"
#include "common/FeatureFlagsEnum.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc exports
#include "render/debug/DebugRenderer_ispc.h"
#else
#include "DebugRenderer.ih"
#endif

namespace ospray {

// Helper functions /////////////////////////////////////////////////////////

static ispc::DebugRendererType typeFromString(const std::string &name)
{
  if (name == "rayDir")
    return ispc::DebugRendererType::RAY_DIR;
  else if (name == "eyeLight")
    return ispc::DebugRendererType::EYE_LIGHT;
  else if (name == "Ng")
    return ispc::DebugRendererType::NG;
  else if (name == "Ns")
    return ispc::DebugRendererType::NS;
  else if (name == "color")
    return ispc::DebugRendererType::COLOR;
  else if (name == "texCoord")
    return ispc::DebugRendererType::TEX_COORD;
  else if (name == "backfacing_Ng")
    return ispc::DebugRendererType::BACKFACING_NG;
  else if (name == "backfacing_Ns")
    return ispc::DebugRendererType::BACKFACING_NS;
  else if (name == "dPds")
    return ispc::DebugRendererType::DPDS;
  else if (name == "dPdt")
    return ispc::DebugRendererType::DPDT;
  else if (name == "primID")
    return ispc::DebugRendererType::PRIM_ID;
  else if (name == "geomID")
    return ispc::DebugRendererType::GEOM_ID;
  else if (name == "instID")
    return ispc::DebugRendererType::INST_ID;
  else if (name == "volume")
    return ispc::DebugRendererType::VOLUME;
  else
    return ispc::DebugRendererType::TEST_FRAME;
}

// DebugRenderer definitions ////////////////////////////////////////////////

DebugRenderer::DebugRenderer(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device)
{}

std::string DebugRenderer::toString() const
{
  return "ospray::DebugRenderer";
}

void DebugRenderer::commit()
{
  Renderer::commit();

  const std::string method = getParam<std::string>("method", "eyeLight");
  getSh()->type = typeFromString(method);
}

AsyncEvent DebugRenderer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *,
    const utility::ArrayView<uint32_t> &taskIDs,
    bool wait) const
{
  AsyncEvent event;
  auto *rendererSh = getSh();
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const size_t numTasks = taskIDs.size();

#ifdef OSPRAY_TARGET_SYCL
  const uint32_t *taskIDsPtr = taskIDs.data();
  event = device.getSyclQueue().submit([&](sycl::handler &cgh) {
    FeatureFlags ff = world->getFeatureFlags();
    ff |= featureFlags;
    ff |= fb->getFeatureFlags();
    ff |= camera->getFeatureFlags();
    cgh.set_specialization_constant<ispc::specFeatureFlags>(ff);

    cgh.set_specialization_constant<debugRendererType>(rendererSh->type);

    const sycl::nd_range<1> dispatchRange =
        device.computeDispatchRange(numTasks, 16);
    cgh.parallel_for(dispatchRange,
        [=](sycl::nd_item<1> taskIndex, sycl::kernel_handler kh) {
          if (taskIndex.get_global_id(0) < numTasks) {
            ispc::FeatureFlagsHandler ffh(kh);
            ispc::DebugRenderer_renderTask(&rendererSh->super,
                fbSh,
                cameraSh,
                worldSh,
                taskIDsPtr,
                taskIndex.get_global_id(0),
                ffh);
          }
        });
  });

  if (wait)
    event.wait_and_throw();

#else
  (void)wait;
  ispc::DebugRenderer_renderTasks(
      &rendererSh->super, fbSh, cameraSh, worldSh, taskIDs.data(), numTasks);
#endif
  return event;
}

} // namespace ospray
