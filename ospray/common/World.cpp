// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "World.h"
#include "api/ISPCDevice.h"
// ispc exports
#include "World_ispc.h"

namespace ospray {

extern "C" void *ospray_getEmbreeDevice()
{
  return api::ISPCDevice::embreeDevice;
}

// Embree helper functions ///////////////////////////////////////////////////

static std::pair<int, int> createEmbreeScenes(RTCScene &geometryScene,
    RTCScene &volumeScene,
    const DataT<Instance *> &instances,
    int embreeFlags)
{
  RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

  int numGeomInstances = 0;
  int numVolumeInstances = 0;

  for (auto &&inst : instances) {
    auto instGeometryScene = inst->group->embreeGeometryScene();
    auto instVolumeScene = inst->group->embreeVolumeScene();

    auto xfm = inst->xfm();

    {
      auto eInst = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);
      rtcSetGeometryInstancedScene(eInst, instGeometryScene.value());
      rtcSetGeometryTransform(eInst, 0, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &xfm);
      rtcCommitGeometry(eInst);

      rtcAttachGeometry(geometryScene, eInst);

#if 0 // NOTE(jda) - there seems to still be an Embree ref-count issue here
        rtcReleaseGeometry(eInst);
#endif

      numGeomInstances++;
    }

    {
      auto eInst = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);
      rtcSetGeometryInstancedScene(eInst, instVolumeScene.value());
      rtcSetGeometryTransform(eInst, 0, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &xfm);
      rtcCommitGeometry(eInst);

      rtcAttachGeometry(volumeScene, eInst);

#if 0 // NOTE(jda) - there seems to still be an Embree ref-count issue here
        rtcReleaseGeometry(eInst);
#endif

      numVolumeInstances++;
    }
  }

  rtcSetSceneFlags(geometryScene, static_cast<RTCSceneFlags>(embreeFlags));
  rtcSetSceneFlags(volumeScene, static_cast<RTCSceneFlags>(embreeFlags));

  return std::make_pair(numGeomInstances, numVolumeInstances);
}

static void freeAndNullifyEmbreeScene(RTCScene &scene)
{
  if (scene)
    rtcReleaseScene(scene);

  scene = nullptr;
}

// World definitions ////////////////////////////////////////////////////////

World::World()
{
  managedObjectType = OSP_WORLD;
  this->ispcEquivalent = ispc::World_create(this);
}

World::~World()
{
  freeAndNullifyEmbreeScene(embreeSceneHandleGeometries);
  freeAndNullifyEmbreeScene(embreeSceneHandleVolumes);
}

std::string World::toString() const
{
  return "ospray::World";
}

void World::commit()
{
  numGeometries = 0;
  numVolumes = 0;

  freeAndNullifyEmbreeScene(embreeSceneHandleGeometries);
  freeAndNullifyEmbreeScene(embreeSceneHandleVolumes);

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
      << "Committing world, which has " << numInstances << " instances";

  RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

  embreeSceneHandleGeometries = rtcNewScene(embreeDevice);
  embreeSceneHandleVolumes = rtcNewScene(embreeDevice);

  if (instances) {
    auto numGeomsAndVolumes = createEmbreeScenes(embreeSceneHandleGeometries,
        embreeSceneHandleVolumes,
        *instances,
        sceneFlags);

    numGeometries = numGeomsAndVolumes.first;
    numVolumes = numGeomsAndVolumes.second;

    instanceIEs = createArrayOfIE(*instances);
  }

  rtcCommitScene(embreeSceneHandleGeometries);
  rtcCommitScene(embreeSceneHandleVolumes);

  ispc::World_set(getIE(),
      instanceIEs.data(),
      numInstances,
      embreeSceneHandleGeometries,
      embreeSceneHandleVolumes);
}

box3f World::getBounds() const
{
  box3f sceneBounds;

  box4f bounds; // NOTE(jda) - Embree expects box4f, NOT box3f...
  rtcGetSceneBounds(embreeSceneHandleGeometries, (RTCBounds *)&bounds);
  sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));

  rtcGetSceneBounds(embreeSceneHandleVolumes, (RTCBounds *)&bounds);
  sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));

  return sceneBounds;
}

OSPTYPEFOR_DEFINITION(World *);

} // namespace ospray
