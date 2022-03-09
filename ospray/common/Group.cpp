// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Group.h"
// ispc exports
#include "common/Group_ispc.h"

namespace ospray {

// Embree helper functions //////////////////////////////////////////////////

inline void createEmbreeScene(
    RTCScene &scene, const DataT<GeometricModel *> &objects, int embreeFlags)
{
  for (auto &&obj : objects) {
    Geometry &geom = obj->geometry();
    rtcAttachGeometry(scene, geom.embreeGeometry);
  }

  rtcSetSceneFlags(scene, static_cast<RTCSceneFlags>(embreeFlags));
}

inline void createEmbreeScene(
    RTCScene &scene, const DataT<VolumetricModel *> &objects, int embreeFlags)
{
  for (auto &&obj : objects) {
    auto geomID = rtcAttachGeometry(scene, obj->embreeGeometryHandle());
    obj->setGeomID(geomID);
  }

  rtcSetSceneFlags(scene, static_cast<RTCSceneFlags>(embreeFlags));
}

static void freeAndNullifyEmbreeScene(RTCScene &scene)
{
  if (scene)
    rtcReleaseScene(scene);

  scene = nullptr;
}

// Group definitions ////////////////////////////////////////////////////////

Group::Group()
{
  managedObjectType = OSP_GROUP;
  this->ispcEquivalent = ispc::Group_create(this);
}

Group::~Group()
{
  freeAndNullifyEmbreeScene(sceneGeometries);
  freeAndNullifyEmbreeScene(sceneVolumes);
  freeAndNullifyEmbreeScene(sceneClippers);
}

std::string Group::toString() const
{
  return "ospray::Group";
}

void Group::commit()
{
  geometricModels = getParamDataT<GeometricModel *>("geometry");
  volumetricModels = getParamDataT<VolumetricModel *>("volume");
  clipModels = getParamDataT<GeometricModel *>("clippingGeometry");

  size_t numGeometries = geometricModels ? geometricModels->size() : 0;
  size_t numVolumes = volumetricModels ? volumetricModels->size() : 0;
  size_t numClippers = clipModels ? clipModels->size() : 0;

  postStatusMsg(OSP_LOG_DEBUG)
      << "=======================================================\n"
      << "Finalizing instance, which has " << numGeometries << " geometries, "
      << numVolumes << " volumes and " << numClippers << " clipping geometries";

  int sceneFlags = RTC_SCENE_FLAG_NONE;
  sceneFlags |=
      (getParam<bool>("dynamicScene", false) ? RTC_SCENE_FLAG_DYNAMIC : 0);
  sceneFlags |=
      (getParam<bool>("compactMode", false) ? RTC_SCENE_FLAG_COMPACT : 0);
  sceneFlags |=
      (getParam<bool>("robustMode", false) ? RTC_SCENE_FLAG_ROBUST : 0);

  freeAndNullifyEmbreeScene(sceneGeometries);
  freeAndNullifyEmbreeScene(sceneVolumes);
  freeAndNullifyEmbreeScene(sceneClippers);

  geometricModelIEs.clear();
  volumetricModelIEs.clear();
  clipModelIEs.clear();

  if (!embreeDevice) {
    throw std::runtime_error("invalid Embree device");
  }

  if (numGeometries > 0) {
    sceneGeometries = rtcNewScene(embreeDevice);

    createEmbreeScene(sceneGeometries, *geometricModels, sceneFlags);
    geometricModelIEs = createArrayOfIE(*geometricModels);

    rtcCommitScene(sceneGeometries);
  }

  if (numVolumes > 0) {
    sceneVolumes = rtcNewScene(embreeDevice);

    createEmbreeScene(sceneVolumes, *volumetricModels, sceneFlags);
    volumetricModelIEs = createArrayOfIE(*volumetricModels);

    rtcCommitScene(sceneVolumes);
  }

  if (numClippers > 0) {
    sceneClippers = rtcNewScene(embreeDevice);

    createEmbreeScene(sceneClippers,
        *clipModels,
        sceneFlags | RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION
            | RTC_SCENE_FLAG_ROBUST);
    clipModelIEs = createArrayOfIE(*clipModels);

    numInvertedClippers = 0;
    for (auto &&obj : *clipModels)
      numInvertedClippers += obj->invertedNormals() ? 1 : 0;

    rtcCommitScene(sceneClippers);
  }

  ispc::Group_set(getIE(),
      geometricModels ? geometricModelIEs.data() : nullptr,
      numGeometries,
      volumetricModels ? volumetricModelIEs.data() : nullptr,
      numVolumes,
      clipModels ? clipModelIEs.data() : nullptr,
      numClippers);
}

box3f Group::getBounds() const
{
  box3f sceneBounds;

  box4f bounds; // NOTE(jda) - Embree expects box4f, NOT box3f...
  if (sceneGeometries) {
    rtcGetSceneBounds(sceneGeometries, (RTCBounds *)&bounds);
    sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));
  }

  if (sceneVolumes) {
    rtcGetSceneBounds(sceneVolumes, (RTCBounds *)&bounds);
    sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));
  }

  return sceneBounds;
}

void Group::setDevice(RTCDevice device)
{
  embreeDevice = device;
}

OSPTYPEFOR_DEFINITION(Group *);

} // namespace ospray
