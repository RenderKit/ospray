// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "GeometricModel.h"
#include "render/Material.h"

namespace ospray {

GeometricModel::GeometricModel(api::ISPCDevice &device, Geometry *_geometry)
    : AddStructShared(device.getIspcrtContext(), device), geomAPI(_geometry)
{
  managedObjectType = OSP_GEOMETRIC_MODEL;
}

std::string GeometricModel::toString() const
{
  return "ospray::GeometricModel";
}

void GeometricModel::commit()
{
  geom =
      hasParam("geometry") ? (Geometry *)getParamObject("geometry") : geomAPI;
  if (!geom)
    throw std::runtime_error(toString() + " received NULL 'geometry'");

  materialData = getParamDataT<Material *>("material", false, true);

  materialArray = nullptr;
  materialIDArray = nullptr;
  getSh()->material = nullptr;
  getSh()->materialID = nullptr;
  getSh()->numMaterials = 0;
  featureFlagsOther = FFO_NONE;
  if (materialData) {
    for (auto &&mat : materialData->as<Material *>())
      featureFlagsOther |= mat->getFeatureFlags().other;

    materialArray = make_buffer_shared_unique<ispc::Material *>(
        getISPCDevice().getIspcrtContext(),
        createArrayOfSh<ispc::Material>(materialData->as<Material *>()));
    getSh()->material = materialArray->sharedPtr();
    getSh()->numMaterials = materialArray->size();
  } else {
    materialData = getParamDataT<uint32_t>("material", false, true);
    if (materialData) {
      materialIDArray = make_buffer_shared_unique<uint32_t>(
          getISPCDevice().getIspcrtContext(),
          materialData->as<uint32_t>().data(),
          materialData->size());
      getSh()->materialID = materialIDArray->sharedPtr();
      getSh()->numMaterials = materialIDArray->size();
    }
  }
  colorData = getParamDataT<vec4f>("color", false, true);
  indexData = getParamDataT<uint8_t>("index");

  size_t maxItems = geom->numPrimitives();
  if (indexData && indexData->size() < maxItems) {
    postStatusMsg(OSP_LOG_INFO)
        << toString()
        << " not enough 'index' elements for 'geometry', clamping";
  }

  if (indexData)
    maxItems = 256; // conservative, should actually go over the index

  if (materialData && materialData->size() > 1
      && materialData->size() < maxItems) {
    postStatusMsg(OSP_LOG_INFO)
        << toString()
        << " potentially not enough 'material' elements for "
           "'geometry', clamping";
  }

  if (colorData && colorData->size() > 1 && colorData->size() < maxItems) {
    postStatusMsg(OSP_LOG_INFO)
        << toString()
        << " potentially not enough 'color' elements for 'geometry', clamping";
  }

  getSh()->geom = geom->getSh();
  getSh()->color = *ispc(colorData);
  getSh()->index = *ispc(indexData);
  getSh()->invertedNormals = getParam<bool>("invertNormals", false);
  getSh()->userID = getParam<uint32>("id", RTC_INVALID_GEOMETRY_ID);
}

bool GeometricModel::hasEmissiveMaterials(
    Ref<const DataT<Material *>> rendererMaterials) const
{
  // No materials used - nothing to check
  if (!materialData)
    return false;

  // Check geometry model materials
  bool hasEmissive = false;
  if (materialArray) {
    for (auto &&mat : materialData->as<Material *>())
      if (mat->isEmissive()) {
        hasEmissive = true;
        break;
      }
  }
  // Check renderer materials referenced from geometry model
  else if (materialIDArray) {
    for (auto matIdx : materialData->as<uint32_t>())
      if ((matIdx < rendererMaterials->size())
          && ((*rendererMaterials)[matIdx]->isEmissive())) {
        hasEmissive = true;
        break;
      }
  }

  // Done
  return hasEmissive;
}

OSPTYPEFOR_DEFINITION(GeometricModel *);

} // namespace ospray
