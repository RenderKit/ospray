// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathTracer.h"
#include "PathTracerData.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "render/Material.h"

#ifdef OSPRAY_TARGET_SYCL
#include "PathTracer.ih"
#include "render/bsdfs/MicrofacetAlbedoTables.ih"
#else
// ispc exports
#include "math/Distribution1D_ispc.h"
#include "render/bsdfs/MicrofacetAlbedoTables_ispc.h"
#include "render/pathtracer/PathTracer_ispc.h"
#endif

namespace ospray {

PathTracer::PathTracer(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtDevice(), device)
{}

std::string PathTracer::toString() const
{
  return "ospray::PathTracer";
}

void PathTracer::commit()
{
  Renderer::commit();

  getSh()->rouletteDepth = getParam<int>("roulettePathLength", 5);
  getSh()->maxRadiance = getParam<float>("maxContribution", inf);
  getSh()->numLightSamples = getParam<int>("lightSamples", -1);

  // Set shadow catcher plane
  const vec4f shadowCatcherPlane =
      getParam<vec4f>("shadowCatcherPlane", vec4f(0.f));
  const vec3f normal = vec3f(shadowCatcherPlane);
  const float l = length(normal);
  getSh()->shadowCatcher = l > 0.f;
  const float rl = rcp(l);
  getSh()->shadowCatcherPlane = vec4f(normal * rl, shadowCatcherPlane.w * rl);

  importanceSampleGeometryLights = getParam<bool>("geometryLights", true);
  getSh()->backgroundRefraction = getParam<bool>("backgroundRefraction", false);
}

void *PathTracer::beginFrame(FrameBuffer *, World *world)
{
  if (!world)
    return nullptr;

  const bool geometryLightListValid =
      importanceSampleGeometryLights == scannedGeometryLights;

  if (world->pathtracerData.get() && geometryLightListValid)
    return nullptr;

  // Create PathTracerData object
  std::unique_ptr<PathTracerData> pathtracerData =
      rkcommon::make_unique<PathTracerData>(
          *world, importanceSampleGeometryLights, *this);
  world->getSh()->pathtracerData = pathtracerData->getSh();
  world->pathtracerData = std::move(pathtracerData);
  scannedGeometryLights = importanceSampleGeometryLights;
  return nullptr;
}

void PathTracer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs
#ifdef OSPRAY_TARGET_SYCL
    ,
    sycl::queue &syclQueue
#endif
) const
{
  auto *rendererSh = getSh();
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const size_t numTasks = taskIDs.size();

#ifdef OSPRAY_TARGET_SYCL
  const uint32_t *taskIDsPtr = taskIDs.data();
  auto event = syclQueue.submit([&](sycl::handler &cgh) {
    const cl::sycl::nd_range<1> dispatchRange =
        computeDispatchRange(numTasks, 16);
    cgh.parallel_for(dispatchRange, [=](cl::sycl::nd_item<1> taskIndex) {
      if (taskIndex.get_global_id(0) < numTasks) {
        ispc::PathTracer_renderTask(&rendererSh->super,
            fbSh,
            cameraSh,
            worldSh,
            perFrameData,
            taskIDsPtr,
            taskIndex.get_global_id(0));
      }
    });
  });
  event.wait_and_throw();
  // For prints we have to flush the entire queue, because other stuff is queued
  syclQueue.wait_and_throw();
#else
  ispc::PathTracer_renderTasks(&rendererSh->super,
      fbSh,
      cameraSh,
      worldSh,
      perFrameData,
      taskIDs.data(),
      numTasks);
#endif
}

} // namespace ospray
