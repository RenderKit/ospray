// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Group.h"
// ispc exports
#include "Group_ispc.h"

namespace ospray {

extern "C" void *ospray_getEmbreeDevice();

// Embree helper functions //////////////////////////////////////////////////

template <typename T>
inline std::vector<void *> createEmbreeScene(
    RTCScene &scene, const DataT<T *> &objects, int embreeFlags)
{
  std::vector<void *> ptrsToIE;
  for (auto &&obj : objects) {
    Geometry &geom = obj->geometry();
    rtcAttachGeometry(scene, geom.embreeGeometry);
    ptrsToIE.push_back(geom.getIE());
  }

  rtcSetSceneFlags(scene, static_cast<RTCSceneFlags>(embreeFlags));

  return ptrsToIE;
}

template <>
inline std::vector<void *> createEmbreeScene<VolumetricModel>(
    RTCScene &scene, const DataT<VolumetricModel *> &objects, int embreeFlags)
{
  for (auto &&obj : objects) {
    auto geomID = rtcAttachGeometry(scene, obj->embreeGeometryHandle());
    obj->setGeomID(geomID);
  }

  rtcSetSceneFlags(scene, static_cast<RTCSceneFlags>(embreeFlags));

  return {};
}

static void freeAndNullifyEmbreeScene(RTCScene &scene)
{
  if (scene)
    rtcReleaseScene(scene);

  scene = nullptr;
}

static void freeIEPtrs(std::vector<void *> &ptrs)
{
  ptrs.clear();
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
  freeIEPtrs(geometryIEs);
  freeIEPtrs(volumeIEs);
}

std::string Group::toString() const
{
  return "ospray::Group";
}

void Group::commit()
{
  geometricModels = getParamDataT<GeometricModel *>("geometry");
  volumetricModels = getParamDataT<VolumetricModel *>("volume");

  // get rid of stride for now
  if (geometricModels && !geometricModels->compact()) {
    auto data =
        new Data(OSP_GEOMETRIC_MODEL, vec3ui(geometricModels->size(), 1, 1));
    data->copy(*geometricModels, vec3ui(0));
    geometricModels = &(data->as<GeometricModel *>());
    data->refDec();
  }
  if (volumetricModels && !volumetricModels->compact()) {
    auto data =
        new Data(OSP_VOLUMETRIC_MODEL, vec3ui(volumetricModels->size(), 1, 1));
    data->copy(*volumetricModels, vec3ui(0));
    volumetricModels = &(data->as<VolumetricModel *>());
    data->refDec();
  }

  size_t numGeometries = geometricModels ? geometricModels->size() : 0;
  size_t numVolumes = volumetricModels ? volumetricModels->size() : 0;

  postStatusMsg(OSP_LOG_DEBUG)
      << "=======================================================\n"
      << "Finalizing instance, which has " << numGeometries
      << " geometries and " << numVolumes << " volumes";

  int sceneFlags = 0;
  sceneFlags |=
      (getParam<bool>("dynamicScene", false) ? RTC_SCENE_FLAG_DYNAMIC : 0);
  sceneFlags |=
      (getParam<bool>("compactMode", false) ? RTC_SCENE_FLAG_COMPACT : 0);
  sceneFlags |=
      (getParam<bool>("robustMode", false) ? RTC_SCENE_FLAG_ROBUST : 0);

  freeAndNullifyEmbreeScene(sceneGeometries);
  freeAndNullifyEmbreeScene(sceneVolumes);

  freeIEPtrs(geometryIEs);
  freeIEPtrs(volumeIEs);

  geometricModelIEs.clear();
  volumetricModelIEs.clear();

  RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

  sceneGeometries = rtcNewScene(embreeDevice);
  sceneVolumes = rtcNewScene(embreeDevice);

  if (numGeometries > 0) {
    geometryIEs = createEmbreeScene<GeometricModel>(
        sceneGeometries, *geometricModels, sceneFlags);
    geometricModelIEs = createArrayOfIE(*geometricModels);
  }

  if (numVolumes > 0) {
    volumeIEs = createEmbreeScene<VolumetricModel>(
        sceneVolumes, *volumetricModels, sceneFlags);
    volumetricModelIEs = createArrayOfIE(*volumetricModels);
  }

  rtcCommitScene(sceneGeometries);
  rtcCommitScene(sceneVolumes);

  ispc::Group_set(getIE(),
      geometricModels ? geometricModelIEs.data() : nullptr,
      numGeometries,
      volumetricModels ? volumetricModelIEs.data() : nullptr,
      numVolumes);
}

box3f Group::getBounds() const
{
  box3f sceneBounds;

  box4f bounds; // NOTE(jda) - Embree expects box4f, NOT box3f...
  rtcGetSceneBounds(sceneGeometries, (RTCBounds *)&bounds);
  sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));

  rtcGetSceneBounds(sceneVolumes, (RTCBounds *)&bounds);
  sceneBounds.extend(box3f(vec3f(&bounds.lower[0]), vec3f(&bounds.upper[0])));

  return sceneBounds;
}

OSPTYPEFOR_DEFINITION(Group *);

} // namespace ospray
