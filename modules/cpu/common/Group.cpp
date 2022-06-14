// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Group.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "volume/VolumetricModel.h"

namespace ospray {

// Embree helper functions //////////////////////////////////////////////////

template <class MODEL>
inline void createEmbreeScene(
    RTCScene &scene, const DataT<MODEL *> &objects, int embreeFlags)
{
  for (auto &&obj : objects)
    rtcAttachGeometry(scene, obj->embreeGeometryHandle());

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
}

Group::~Group()
{
  freeAndNullifyEmbreeScene(sceneGeometries);
  freeAndNullifyEmbreeScene(sceneVolumes);
  freeAndNullifyEmbreeScene(sceneClippers);

  BufferSharedDelete(getSh()->geometricModels);
  BufferSharedDelete(getSh()->volumetricModels);
  BufferSharedDelete(getSh()->clipModels);
  getSh()->geometricModels = nullptr;
  getSh()->volumetricModels = nullptr;
  getSh()->clipModels = nullptr;
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
  lights = getParamDataT<Light *>("light");

  size_t numGeometries = geometricModels ? geometricModels->size() : 0;
  size_t numVolumes = volumetricModels ? volumetricModels->size() : 0;
  size_t numClippers = clipModels ? clipModels->size() : 0;
  size_t numLights = lights ? lights->size() : 0;

  postStatusMsg(OSP_LOG_DEBUG)
      << "=======================================================\n"
      << "Finalizing instance, which has " << numGeometries << " geometries, "
      << numVolumes << " volumes, " << numClippers
      << " clipping geometries and " << numLights << " lights";

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

  BufferSharedDelete(getSh()->geometricModels);
  BufferSharedDelete(getSh()->volumetricModels);
  BufferSharedDelete(getSh()->clipModels);
  getSh()->geometricModels = nullptr;
  getSh()->volumetricModels = nullptr;
  getSh()->clipModels = nullptr;

  if (!embreeDevice) {
    throw std::runtime_error("invalid Embree device");
  }

  if (numGeometries > 0) {
    sceneGeometries = rtcNewScene(embreeDevice);

    createEmbreeScene(sceneGeometries, *geometricModels, sceneFlags);
    getSh()->geometricModels =
        BufferSharedCreate<ispc::GeometricModel *>(numGeometries,
            createArrayOfSh<ispc::GeometricModel>(*geometricModels).data());

    rtcCommitScene(sceneGeometries);
  }

  if (numVolumes > 0) {
    sceneVolumes = rtcNewScene(embreeDevice);

    createEmbreeScene(sceneVolumes, *volumetricModels, sceneFlags);
    getSh()->volumetricModels =
        BufferSharedCreate<ispc::VolumetricModel *>(numVolumes,
            createArrayOfSh<ispc::VolumetricModel>(*volumetricModels).data());

    rtcCommitScene(sceneVolumes);
  }

  if (numClippers > 0) {
    sceneClippers = rtcNewScene(embreeDevice);

    createEmbreeScene(sceneClippers,
        *clipModels,
        sceneFlags | RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION
            | RTC_SCENE_FLAG_ROBUST);
    getSh()->clipModels = BufferSharedCreate<ispc::GeometricModel *>(
        numClippers, createArrayOfSh<ispc::GeometricModel>(*clipModels).data());

    numInvertedClippers = 0;
    for (auto &&obj : *clipModels)
      numInvertedClippers += obj->invertedNormals() ? 1 : 0;

    rtcCommitScene(sceneClippers);
  }

  getSh()->numGeometricModels = numGeometries;
  getSh()->numVolumetricModels = numVolumes;
  getSh()->numClipModels = numClippers;

  // Create empty scene for lights-only group,
  // it is needed to have rtcGeometry created in Instance object
  // which in turn is needed for motion blur matrices interpolation
  if ((numLights > 0) && (numGeometries == 0) && (numVolumes == 0)
      && (numClippers == 0)) {
    sceneGeometries = rtcNewScene(embreeDevice);
    rtcCommitScene(sceneGeometries);
  }
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
