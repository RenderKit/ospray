// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "World.h"
#include "Instance.h"
#include "common/FeatureFlagsEnum.h"
#include "lights/Light.h"
#include "render/pathtracer/PathTracerData.h"
#include "render/scivis/SciVisData.h"

namespace ospray {

// Embree helper functions ///////////////////////////////////////////////////

static void addGeometryInstance(RTCScene &scene,
    RTCScene instScene,
    Instance *inst,
    RTCDevice embreeDevice,
    unsigned int id)
{
  if (!embreeDevice)
    throw std::runtime_error("invalid Embree device");

  // Create parent scene if not yet created
  if (!scene)
    scene = rtcNewScene(embreeDevice);

  // Create geometry instance
  auto eInst = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);
  rtcSetGeometryInstancedScene(eInst, instScene);
  inst->setEmbreeGeom(eInst);

  rtcAttachGeometryByID(scene, eInst, id);
  rtcReleaseGeometry(eInst);
}

static void freeAndNullifyEmbreeScene(RTCScene &scene)
{
  if (scene)
    rtcReleaseScene(scene);

  scene = nullptr;
}

// World definitions ////////////////////////////////////////////////////////

World::~World()
{
  // Release Embree scenes
  freeAndNullifyEmbreeScene(getSh()->embreeSceneHandleGeometries);
#ifdef OSPRAY_ENABLE_VOLUMES
  freeAndNullifyEmbreeScene(getSh()->embreeSceneHandleVolumes);
#endif
#ifndef OSPRAY_TARGET_SYCL
  freeAndNullifyEmbreeScene(getSh()->embreeSceneHandleClippers);
#endif
}

World::World(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device)
{
  managedObjectType = OSP_WORLD;
}

std::string World::toString() const
{
  return "ospray::World";
}

void World::commit()
{
  RTCScene &esGeom = getSh()->embreeSceneHandleGeometries;
#ifdef OSPRAY_ENABLE_VOLUMES
  RTCScene &esVol = getSh()->embreeSceneHandleVolumes;
#endif
#ifndef OSPRAY_TARGET_SYCL
  RTCScene &esClip = getSh()->embreeSceneHandleClippers;
#endif

  freeAndNullifyEmbreeScene(esGeom);
#ifdef OSPRAY_ENABLE_VOLUMES
  freeAndNullifyEmbreeScene(esVol);
#endif
#ifndef OSPRAY_TARGET_SYCL
  freeAndNullifyEmbreeScene(esClip);
#endif

  scivisData = nullptr;
  pathtracerData = nullptr;

  instances = getParamDataT<Instance *>("instance");
  lights = getParamDataT<Light *>("light");

  auto numInstances = instances ? instances->size() : 0;

  int sceneFlags = RTC_SCENE_FLAG_NONE;
  RTCBuildQuality buildQuality = RTC_BUILD_QUALITY_HIGH;
  if (getParam<bool>("dynamicScene", false)) {
    sceneFlags |= RTC_SCENE_FLAG_DYNAMIC;
    buildQuality = RTC_BUILD_QUALITY_LOW;
  }
  sceneFlags |=
      (getParam<bool>("compactMode", false) ? RTC_SCENE_FLAG_COMPACT : 0);
  sceneFlags |=
      (getParam<bool>("robustMode", false) ? RTC_SCENE_FLAG_ROBUST : 0);

  postStatusMsg(OSP_LOG_DEBUG)
      << "=======================================================\n"
      << "Committing world, which has " << numInstances << " instances and "
      << (lights ? lights->size() : 0) << " lights";

  instanceArray = nullptr;
  getSh()->numInvertedClippers = 0;

  RTCDevice embreeDevice = getISPCDevice().getEmbreeDevice();
  if (instances) {
    for (auto &&inst : *instances)
#ifndef OSPRAY_TARGET_SYCL
      if (inst->group->sceneClippers)
        getSh()->numInvertedClippers += inst->group->numInvertedClippers;
#endif

    // Create shared buffers for instance pointers
    instanceArray = make_buffer_shared_unique<ispc::Instance *>(
        getISPCDevice().getIspcrtContext(),
        sizeof(ispc::Instance *) * numInstances);
    getSh()->instances = instanceArray->sharedPtr();

    // Populate shared buffer with instance pointers,
    // create Embree instances
    featureFlags.reset();
    unsigned int id = 0;
    for (auto &&inst : *instances) {
      getSh()->instances[id] = inst->getSh();
      if (inst->group->sceneGeometries) {
        addGeometryInstance(
            esGeom, inst->group->sceneGeometries, inst, embreeDevice, id);
      }
#ifdef OSPRAY_ENABLE_VOLUMES
      if (inst->group->sceneVolumes) {
        featureFlags.other |= FFO_VOLUME_IN_SCENE;
        addGeometryInstance(
            esVol, inst->group->sceneVolumes, inst, embreeDevice, id);
      }
#endif
#ifndef OSPRAY_TARGET_SYCL
      if (inst->group->sceneClippers) {
        addGeometryInstance(
            esClip, inst->group->sceneClippers, inst, embreeDevice, id);
      }
#endif
      // Gather feature flags from all groups
      const FeatureFlags &gff = inst->group->getFeatureFlags();
      featureFlags |= gff;
      id++;
    }
  }

  // Gather light types
  if (lights) {
    for (auto &&light : *lights) {
      featureFlags |= light->getFeatureFlags();
    }
  }

  if (esGeom) {
    rtcSetSceneFlags(esGeom, static_cast<RTCSceneFlags>(sceneFlags));
    rtcSetSceneBuildQuality(esGeom, buildQuality);
    rtcCommitScene(esGeom);
  }
#ifdef OSPRAY_ENABLE_VOLUMES
  if (esVol) {
    rtcSetSceneFlags(esVol, static_cast<RTCSceneFlags>(sceneFlags));
    rtcSetSceneBuildQuality(esVol, buildQuality);
    rtcCommitScene(esVol);
  }
#endif
#ifndef OSPRAY_TARGET_SYCL
  if (esClip) {
    rtcSetSceneFlags(esClip,
        static_cast<RTCSceneFlags>(
            sceneFlags | RTC_SCENE_FLAG_FILTER_FUNCTION_IN_ARGUMENTS));
    rtcSetSceneBuildQuality(esClip, buildQuality);
    rtcCommitScene(esClip);
  }
#endif
}

box3f World::getBounds() const
{
  box3f sceneBounds;

  box4f bounds; // NOTE(jda) - Embree expects box4f, NOT box3f...
  if (getSh()->embreeSceneHandleGeometries) {
    rtcGetSceneBounds(
        getSh()->embreeSceneHandleGeometries, (RTCBounds *)&bounds);
    sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));
  }

#ifdef OSPRAY_ENABLE_VOLUMES
  if (getSh()->embreeSceneHandleVolumes) {
    rtcGetSceneBounds(getSh()->embreeSceneHandleVolumes, (RTCBounds *)&bounds);
    sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));
  }
#endif

  return sceneBounds;
}

OSPTYPEFOR_DEFINITION(World *);

} // namespace ospray
