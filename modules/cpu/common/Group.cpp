// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Group.h"
#include "ISPCDevice.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "volume/VolumetricModel.h"
#endif

namespace ospray {

// Embree helper functions //////////////////////////////////////////////////

template <class MODEL>
inline void createEmbreeScene(RTCScene &scene,
    FeatureFlags &featureFlags,
    const DataT<MODEL *> &objects,
    const int embreeFlags,
    const RTCBuildQuality buildQuality)
{
  for (auto &&obj : objects) {
    rtcAttachGeometry(scene, obj->embreeGeometryHandle());
    featureFlags |= obj->getFeatureFlags();
  }

  rtcSetSceneFlags(scene, static_cast<RTCSceneFlags>(embreeFlags));
  rtcSetSceneBuildQuality(scene, buildQuality);
}

static void freeAndNullifyEmbreeScene(RTCScene &scene)
{
  if (scene)
    rtcReleaseScene(scene);

  scene = nullptr;
}

// Group definitions ////////////////////////////////////////////////////////

Group::Group(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device)
{
  managedObjectType = OSP_GROUP;
}

Group::~Group()
{
  freeAndNullifyEmbreeScene(sceneGeometries);
#ifdef OSPRAY_ENABLE_VOLUMES
  freeAndNullifyEmbreeScene(sceneVolumes);
#endif
  freeAndNullifyEmbreeScene(sceneClippers);

  getSh()->geometricModels = nullptr;
#ifdef OSPRAY_ENABLE_VOLUMES
  getSh()->volumetricModels = nullptr;
#endif
  getSh()->clipModels = nullptr;
}

std::string Group::toString() const
{
  return "ospray::Group";
}

void Group::commit()
{
  geometricModels = getParamDataT<GeometricModel *>("geometry");
#ifdef OSPRAY_ENABLE_VOLUMES
  volumetricModels = getParamDataT<VolumetricModel *>("volume");
#endif
  clipModels = getParamDataT<GeometricModel *>("clippingGeometry");
  lights = getParamDataT<Light *>("light");

  size_t numGeometries = geometricModels ? geometricModels->size() : 0;
#ifdef OSPRAY_ENABLE_VOLUMES
  size_t numVolumes = volumetricModels ? volumetricModels->size() : 0;
#endif
  size_t numClippers = clipModels ? clipModels->size() : 0;
  size_t numLights = lights ? lights->size() : 0;

  postStatusMsg(OSP_LOG_DEBUG)
      << "=======================================================\n"
      << "Finalizing instance, which has " << numGeometries << " geometries, "
#ifdef OSPRAY_ENABLE_VOLUMES
      << numVolumes << " volumes, "
#endif
      << numClippers << " clipping geometries and " << numLights << " lights";

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

  freeAndNullifyEmbreeScene(sceneGeometries);
#ifdef OSPRAY_ENABLE_VOLUMES
  freeAndNullifyEmbreeScene(sceneVolumes);
#endif
  freeAndNullifyEmbreeScene(sceneClippers);

  geometricModelsArray = nullptr;
#ifdef OSPRAY_ENABLE_VOLUMES
  volumetricModelsArray = nullptr;
#endif
  clipModelsArray = nullptr;

  getSh()->geometricModels = nullptr;
#ifdef OSPRAY_ENABLE_VOLUMES
  getSh()->volumetricModels = nullptr;
#endif
  getSh()->clipModels = nullptr;

  RTCDevice embreeDevice = getISPCDevice().getEmbreeDevice();
  if (!embreeDevice) {
    throw std::runtime_error("invalid Embree device");
  }

  featureFlags.reset();
  if (numGeometries > 0) {
    sceneGeometries = rtcNewScene(embreeDevice);
    createEmbreeScene(sceneGeometries,
        featureFlags,
        *geometricModels,
        sceneFlags,
        buildQuality);

    geometricModelsArray = make_buffer_shared_unique<ispc::GeometricModel *>(
        getISPCDevice().getIspcrtContext(),
        createArrayOfSh<ispc::GeometricModel>(*geometricModels));
    getSh()->geometricModels = geometricModelsArray->sharedPtr();

    rtcCommitScene(sceneGeometries);
  }

#ifdef OSPRAY_ENABLE_VOLUMES
  if (numVolumes > 0) {
    sceneVolumes = rtcNewScene(embreeDevice);
    createEmbreeScene(sceneVolumes,
        featureFlags,
        *volumetricModels,
        sceneFlags,
        buildQuality);

    volumetricModelsArray = make_buffer_shared_unique<ispc::VolumetricModel *>(
        getISPCDevice().getIspcrtContext(),
        createArrayOfSh<ispc::VolumetricModel>(*volumetricModels));
    getSh()->volumetricModels = volumetricModelsArray->sharedPtr();

    rtcCommitScene(sceneVolumes);
  }
#endif

  if (numClippers > 0) {
    sceneClippers = rtcNewScene(embreeDevice);
    createEmbreeScene(sceneClippers,
        featureFlags,
        *clipModels,
        sceneFlags | RTC_SCENE_FLAG_FILTER_FUNCTION_IN_ARGUMENTS
            | RTC_SCENE_FLAG_ROBUST,
        buildQuality);

    clipModelsArray = make_buffer_shared_unique<ispc::GeometricModel *>(
        getISPCDevice().getIspcrtContext(),
        createArrayOfSh<ispc::GeometricModel>(*clipModels));
    getSh()->clipModels = clipModelsArray->sharedPtr();

    numInvertedClippers = 0;
    for (auto &&obj : *clipModels)
      numInvertedClippers += obj->invertedNormals() ? 1 : 0;

    rtcCommitScene(sceneClippers);
  }

  getSh()->numGeometricModels = numGeometries;
#ifdef OSPRAY_ENABLE_VOLUMES
  getSh()->numVolumetricModels = numVolumes;
#endif
  getSh()->numClipModels = numClippers;

  if (numLights > 0) {
    // Gather light types
    for (auto &&light : *lights)
      featureFlags |= light->getFeatureFlags();

    // Create empty scene for lights-only group,
    // it is needed to have rtcGeometry created in Instance object
    // which in turn is needed for motion blur matrices interpolation
    if ((numGeometries == 0)
#ifdef OSPRAY_ENABLE_VOLUMES
        && (numVolumes == 0)
#endif
        && (numClippers == 0)) {
      sceneGeometries = rtcNewScene(embreeDevice);
      rtcCommitScene(sceneGeometries);
    }
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

#ifdef OSPRAY_ENABLE_VOLUMES
  if (sceneVolumes) {
    rtcGetSceneBounds(sceneVolumes, (RTCBounds *)&bounds);
    sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));
  }
#endif

  return sceneBounds;
}

OSPTYPEFOR_DEFINITION(Group *);

} // namespace ospray
