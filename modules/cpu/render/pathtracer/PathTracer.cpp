// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathTracer.h"
#include "PathTracerData.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
// ispc exports
#include "render/bsdfs/MicrofacetAlbedoTables_ispc.h"
#include "render/pathtracer/PathTracer_ispc.h"

namespace ospray {

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

  ispc::precomputeMicrofacetAlbedoTables();
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

} // namespace ospray
