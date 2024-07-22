// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "DebugRenderer.h"
#include "camera/Camera.h"
#include "common/DeviceRT.h"
#include "common/FeatureFlagsEnum.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"

// Kernel launcher is defined in another compilation unit
DECLARE_RENDERER_KERNEL_LAUNCHER(DebugRenderer_renderTaskLauncher);

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
    : AddStructShared(device.getDRTDevice(), device)
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

devicert::AsyncEvent DebugRenderer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  // Gather feature flags from all components
  FeatureFlags ff = world->getFeatureFlags();
  ff |= featureFlags;
  ff |= fb->getFeatureFlags();
  ff |= camera->getFeatureFlags();

  // Prepare parameters for kernel launch
  auto *rendererSh = &getSh()->super;
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const size_t numTasks = taskIDs.size();
  const vec2i taskSize = fb->getRenderTaskSize();
  const vec3ui itemDims = vec3ui(taskSize.x, taskSize.y, numTasks);

  // Launch rendering kernel on the device
  return drtDevice.launchRendererKernel(itemDims,
      ispc::DebugRenderer_renderTaskLauncher,
      rendererSh,
      fbSh,
      cameraSh,
      worldSh,
      taskIDs.data(),
      ff);
}

} // namespace ospray
