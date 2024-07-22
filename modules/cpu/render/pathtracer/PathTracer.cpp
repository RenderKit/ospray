// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathTracer.h"
#include "PathTracerData.h"
#include "camera/Camera.h"
#include "common/DeviceRT.h"
#include "common/FeatureFlagsEnum.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "render/Material.h"

// Kernel launcher is defined in another compilation unit
DECLARE_RENDERER_KERNEL_LAUNCHER(PathTracer_renderTaskLauncher);

namespace ospray {

PathTracer::PathTracer(api::ISPCDevice &device)
    : AddStructShared(device.getDRTDevice(), device)
{}

std::string PathTracer::toString() const
{
  return "ospray::PathTracer";
}

void PathTracer::commit()
{
  Renderer::commit();

  getSh()->rouletteDepth = getParam<int>("roulettePathLength", 5);
  getSh()->maxScatteringEvents = getParam<int>("maxScatteringEvents", 20);
  getSh()->maxRadiance = getParam<float>("maxContribution", inf);
  getSh()->numLightSamples = getParam<int>("lightSamples", -1);
  getSh()->limitIndirectLightSamples =
      getParam<bool>("limitIndirectLightSamples", true);

  // Set shadow catcher plane
  const vec4f shadowCatcherPlane =
      getParam<vec4f>("shadowCatcherPlane", vec4f(0.f));
  const vec3f normal = vec3f(shadowCatcherPlane);
  const float l = length(normal);
  getSh()->shadowCatcher = l > 0.f;
  const float rl = rcp(l);
  getSh()->shadowCatcherPlane = vec4f(normal * rl, shadowCatcherPlane.w * rl);

  scanForGeometryLights = getParam<bool>("geometryLights", true);
  getSh()->backgroundRefraction = getParam<bool>("backgroundRefraction", false);
}

void *PathTracer::beginFrame(FrameBuffer *, World *world)
{
  if (!world)
    return nullptr;

  if (!world->pathtracerData
      || (scanForGeometryLights
          && !world->pathtracerData->scannedForGeometryLights)) {
    // Create PathTracerData object
    std::unique_ptr<PathTracerData> pathtracerData =
        rkcommon::make_unique<PathTracerData>(
            *world, scanForGeometryLights, *this);

    world->getSh()->pathtracerData = pathtracerData->getSh();
    world->pathtracerData = std::move(pathtracerData);
  }

  return nullptr;
}

devicert::AsyncEvent PathTracer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  // Gather feature flags from all components
  FeatureFlags ff = world->getFeatureFlags();
  if (world->pathtracerData->getSh()->numGeoLights)
    ff.other |= FFO_LIGHT_GEOMETRY;
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
      ispc::PathTracer_renderTaskLauncher,
      rendererSh,
      fbSh,
      cameraSh,
      worldSh,
      taskIDs.data(),
      ff);
}

} // namespace ospray
