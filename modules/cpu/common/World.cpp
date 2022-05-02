// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "World.h"
#include "Instance.h"
#include "lights/Light.h"

namespace ospray {

// Embree helper functions ///////////////////////////////////////////////////

static void addGeometryInstance(
    RTCScene &scene, RTCScene instScene, Instance *inst, RTCDevice embreeDevice)
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

  rtcAttachGeometry(scene, eInst);
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
  freeAndNullifyEmbreeScene(getSh()->embreeSceneHandleVolumes);
  freeAndNullifyEmbreeScene(getSh()->embreeSceneHandleClippers);

  // Release instances arrays
  BufferSharedDelete(getSh()->geometriesInst);
  BufferSharedDelete(getSh()->volumesInst);
  BufferSharedDelete(getSh()->clippersInst);
  getSh()->geometriesInst = nullptr;
  getSh()->volumesInst = nullptr;
  getSh()->clippersInst = nullptr;

  // Release renderers data
  getSh()->scivisData.destroy();
  getSh()->pathtracerData.destroy();
}

World::World()
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
  RTCScene &esVol = getSh()->embreeSceneHandleVolumes;
  RTCScene &esClip = getSh()->embreeSceneHandleClippers;

  freeAndNullifyEmbreeScene(esGeom);
  freeAndNullifyEmbreeScene(esVol);
  freeAndNullifyEmbreeScene(esClip);

  scivisDataValid = false;
  pathtracerDataValid = false;

  instances = getParamDataT<Instance *>("instance");
  lights = getParamDataT<Light *>("light");

  auto numInstances = instances ? instances->size() : 0;

  int sceneFlags = 0;
  sceneFlags |=
      (getParam<bool>("dynamicScene", false) ? RTC_SCENE_FLAG_DYNAMIC : 0);
  sceneFlags |=
      (getParam<bool>("compactMode", false) ? RTC_SCENE_FLAG_COMPACT : 0);
  sceneFlags |=
      (getParam<bool>("robustMode", false) ? RTC_SCENE_FLAG_ROBUST : 0);

  postStatusMsg(OSP_LOG_DEBUG)
      << "=======================================================\n"
      << "Committing world, which has " << numInstances << " instances and "
      << (lights ? lights->size() : 0) << " lights";

  BufferSharedDelete(getSh()->geometriesInst);
  BufferSharedDelete(getSh()->volumesInst);
  BufferSharedDelete(getSh()->clippersInst);
  getSh()->volumesInst = nullptr;
  getSh()->geometriesInst = nullptr;
  getSh()->clippersInst = nullptr;

  // Calculate number of each instance type
  getSh()->numGeometriesInst = 0;
  getSh()->numVolumesInst = 0;
  getSh()->numClippersInst = 0;
  getSh()->numInvertedClippers = 0;
  if (instances) {
    for (auto &&inst : *instances) {
      if (inst->group->sceneGeometries)
        getSh()->numGeometriesInst++;
      if (inst->group->sceneVolumes)
        getSh()->numVolumesInst++;
      if (inst->group->sceneClippers) {
        getSh()->numClippersInst++;
        getSh()->numInvertedClippers += inst->group->numInvertedClippers;
      }
    }
  }

  // Create shared buffers for instance pointers
  if (getSh()->numGeometriesInst)
    getSh()->geometriesInst = (ispc::Instance **)BufferSharedCreate(
        sizeof(ispc::Instance *) * getSh()->numGeometriesInst);
  if (getSh()->numVolumesInst)
    getSh()->volumesInst = (ispc::Instance **)BufferSharedCreate(
        sizeof(ispc::Instance *) * getSh()->numVolumesInst);
  if (getSh()->numClippersInst)
    getSh()->clippersInst = (ispc::Instance **)BufferSharedCreate(
        sizeof(ispc::Instance *) * getSh()->numClippersInst);

  // Populate shared buffer with instance pointers,
  // create Embree instances
  if (instances) {
    unsigned int gId = 0, vId = 0, cId = 0;
    for (auto &&inst : *instances) {
      if (inst->group->sceneGeometries) {
        getSh()->geometriesInst[gId++] = inst->getSh();
        addGeometryInstance(
            esGeom, inst->group->sceneGeometries, inst, embreeDevice);
      }
      if (inst->group->sceneVolumes) {
        getSh()->volumesInst[vId++] = inst->getSh();
        addGeometryInstance(
            esVol, inst->group->sceneVolumes, inst, embreeDevice);
      }
      if (inst->group->sceneClippers) {
        getSh()->clippersInst[cId++] = inst->getSh();
        addGeometryInstance(
            esClip, inst->group->sceneClippers, inst, embreeDevice);
      }
    }
  }

  if (esGeom) {
    rtcSetSceneFlags(esGeom, static_cast<RTCSceneFlags>(sceneFlags));
    rtcCommitScene(esGeom);
  }
  if (esVol) {
    rtcSetSceneFlags(esVol, static_cast<RTCSceneFlags>(sceneFlags));
    rtcCommitScene(esVol);
  }
  if (esClip) {
    rtcSetSceneFlags(esClip,
        static_cast<RTCSceneFlags>(
            sceneFlags | RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION));
    rtcCommitScene(esClip);
  }
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

  if (getSh()->embreeSceneHandleVolumes) {
    rtcGetSceneBounds(getSh()->embreeSceneHandleVolumes, (RTCBounds *)&bounds);
    sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));
  }

  return sceneBounds;
}

void World::setDevice(RTCDevice device)
{
  embreeDevice = device;
}

OSPTYPEFOR_DEFINITION(World *);

} // namespace ospray
