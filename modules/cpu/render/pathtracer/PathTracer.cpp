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

#ifdef OSPRAY_TARGET_DPCPP
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

  // ispc::precomputeMicrofacetAlbedoTables();
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

#ifdef OSPRAY_TARGET_DPCPP
void PathTracer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs,
    sycl::queue &syclQueue) const
{
  auto *rendererSh = getSh();
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const uint32_t *taskIDsPtr = taskIDs.data();
  const size_t numTasks = taskIDs.size();

  auto event = syclQueue.submit([&](sycl::handler &cgh) {
    const cl::sycl::nd_range<1> dispatchRange =
        computeDispatchRange(numTasks, RTC_SYCL_SIMD_WIDTH);
    cgh.parallel_for(
        dispatchRange, [=](cl::sycl::nd_item<1> taskIndex) RTC_SYCL_KERNEL {
          if (taskIndex.get_global_id(0) < numTasks) {
#if 1
          // Needed for DPC++ prints to work around issue with print in deeper
          // indirect called functions (see JIRA
          // https://jira.devtools.intel.com/browse/XDEPS-4729)
          if (taskIndex.get_global_id(0) == 0) {
            cl::sycl::ext::oneapi::experimental::printf("");
          }
#endif
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
}
#else
void PathTracer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  ispc::PathTracer_renderTasks(getSh(),
      fb->getSh(),
      camera->getSh(),
      world->getSh(),
      perFrameData,
      taskIDs.data(),
      taskIDs.size());
}
#endif

} // namespace ospray
