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
  rtcAttachGeometryByID(scene, eInst, id);
  inst->setEmbreeGeom(scene, id);
  rtcReleaseGeometry(eInst);
}

static void freeAndNullifyEmbreeScene(
    RTCScene &scene, RTCTraversable &traversable)
{
  if (scene)
    rtcReleaseScene(scene);

  scene = nullptr;
  traversable = nullptr;
}

// World definitions ////////////////////////////////////////////////////////

World::~World()
{
  // Release Embree scenes
  freeAndNullifyEmbreeScene(
      embreeSceneHandleGeometries, getSh()->embreeTraversableHandleGeometries);
#ifdef OSPRAY_ENABLE_VOLUMES
  freeAndNullifyEmbreeScene(
      embreeSceneHandleVolumes, getSh()->embreeTraversableHandleVolumes);
#endif
#ifndef OSPRAY_TARGET_SYCL
  freeAndNullifyEmbreeScene(
      embreeSceneHandleClippers, getSh()->embreeTraversableHandleClippers);
#endif
}

World::World(api::ISPCDevice &device)
    : AddStructShared(device.getDRTDevice(), device)
{
  managedObjectType = OSP_WORLD;
}

std::string World::toString() const
{
  return "ospray::World";
}

void World::commit()
{
  RTCScene &esGeom = embreeSceneHandleGeometries;
  freeAndNullifyEmbreeScene(esGeom, getSh()->embreeTraversableHandleGeometries);
#ifdef OSPRAY_ENABLE_VOLUMES
  RTCScene &esVol = embreeSceneHandleVolumes;
  freeAndNullifyEmbreeScene(esVol, getSh()->embreeTraversableHandleVolumes);
#endif
#ifndef OSPRAY_TARGET_SYCL
  RTCScene &esClip = embreeSceneHandleClippers;
  freeAndNullifyEmbreeScene(esClip, getSh()->embreeTraversableHandleClippers);
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
    // Create shared buffers for instance pointers
    instanceArray = devicert::make_buffer_shared_unique<ispc::Instance *>(
        getISPCDevice().getDRTDevice(),
        sizeof(ispc::Instance *) * numInstances);
    getSh()->instances = instanceArray->sharedPtr();

    // Phase 1
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
        addGeometryInstance(
            esVol, inst->group->sceneVolumes, inst, embreeDevice, id);
      }
#endif
#ifndef OSPRAY_TARGET_SYCL
      if (inst->group->sceneClippers) {
        getSh()->numInvertedClippers += inst->group->numInvertedClippers;
        addGeometryInstance(
            esClip, inst->group->sceneClippers, inst, embreeDevice, id);
      }
#endif
      // Gather feature flags from all groups
      const FeatureFlags &gff = inst->group->getFeatureFlags();
      if (inst->motionTransform.motionBlur)
        featureFlags.geometry |= FFG_MOTION_BLUR;
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
    getSh()->embreeTraversableHandleGeometries = rtcGetSceneTraversable(esGeom);
  }
#ifdef OSPRAY_ENABLE_VOLUMES
  if (esVol) {
    rtcSetSceneFlags(esVol, static_cast<RTCSceneFlags>(sceneFlags));
    rtcSetSceneBuildQuality(esVol, buildQuality);
    rtcCommitScene(esVol);
    getSh()->embreeTraversableHandleVolumes = rtcGetSceneTraversable(esVol);
  }
#endif
#ifndef OSPRAY_TARGET_SYCL
  if (esClip) {
    rtcSetSceneFlags(esClip,
        static_cast<RTCSceneFlags>(
            sceneFlags | RTC_SCENE_FLAG_FILTER_FUNCTION_IN_ARGUMENTS));
    rtcSetSceneBuildQuality(esClip, buildQuality);
    rtcCommitScene(esClip);
    getSh()->embreeTraversableHandleClippers = rtcGetSceneTraversable(esClip);
  }
#endif

  if (instances) {
    // Phase 2: set traversable at instances
    // Note: the same instance transform is set in each scene, the instance
    // only needs to access one for interpolation (thus fine to overwrite)
    for (auto &&inst : *instances) {
      if (inst->group->sceneGeometries)
        inst->setEmbreeGeom(getSh()->embreeTraversableHandleGeometries);
#ifdef OSPRAY_ENABLE_VOLUMES
      if (inst->group->sceneVolumes)
        inst->setEmbreeGeom(getSh()->embreeTraversableHandleVolumes);
#endif
#ifndef OSPRAY_TARGET_SYCL
      if (inst->group->sceneClippers)
        inst->setEmbreeGeom(getSh()->embreeTraversableHandleClippers);
#endif
    }
  }
}

box3f World::getBounds() const
{
  box3f sceneBounds;

  box4f bounds; // NOTE(jda) - Embree expects box4f, NOT box3f...
  if (embreeSceneHandleGeometries) {
    rtcGetSceneBounds(embreeSceneHandleGeometries, (RTCBounds *)&bounds);
    sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));
  }

#ifdef OSPRAY_ENABLE_VOLUMES
  if (embreeSceneHandleVolumes) {
    rtcGetSceneBounds(embreeSceneHandleVolumes, (RTCBounds *)&bounds);
    sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));
  }
#endif

  return sceneBounds;
}

OSPTYPEFOR_DEFINITION(World *);

} // namespace ospray
