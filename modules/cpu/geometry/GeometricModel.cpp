// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "GeometricModel.h"
#include "render/Material.h"

namespace ospray {

GeometricModel::GeometricModel(Geometry *_geometry) : geomAPI(_geometry)
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

  bool useRendererMaterialList = false;
  materialData = getParamDataT<Material *>("material", false, true);
  if (materialData) {
    ispcMaterialPtrs =
        createArrayOfSh<ispc::Material>(materialData->as<Material *>());

    auto *data = new Data(ispcMaterialPtrs.data(),
        OSP_VOID_PTR,
        vec3ui(ispcMaterialPtrs.size(), 1, 1),
        vec3l(0));
    materialData = data;
    data->refDec();
  } else {
    ispcMaterialPtrs.clear();
    materialData = getParamDataT<uint32_t>("material", false, true);
    useRendererMaterialList = true;
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
  getSh()->material = *ispc(materialData);
  getSh()->useRendererMaterialList = useRendererMaterialList;
  getSh()->invertedNormals = getParam<bool>("invertNormals", false);
  getSh()->userID = getParam<uint32>("id", RTC_INVALID_GEOMETRY_ID);
}

OSPTYPEFOR_DEFINITION(GeometricModel *);

} // namespace ospray
