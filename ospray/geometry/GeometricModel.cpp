// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "GeometricModel.h"
#include "../render/Material.h"
// ispc exports
#include "GeometricModel_ispc.h"
#include "Geometry_ispc.h"

namespace ospray {

GeometricModel::GeometricModel(Geometry *_geometry)
{
  managedObjectType = OSP_GEOMETRIC_MODEL;
  geom = _geometry;
  this->ispcEquivalent = ispc::GeometricModel_create(this);
}

std::string GeometricModel::toString() const
{
  return "ospray::GeometricModel";
}

void GeometricModel::commit()
{
  bool useRendererMaterialList = false;
  materialData = getParamDataT<Material *>("material", false, true);
  if (materialData) {
    ispcMaterialPtrs = createArrayOfIE(materialData->as<Material *>());

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
        << toString() << " not enough 'index' elements for geometry, clamping";
  }

  if (indexData)
    maxItems = 256; // conservative, should actually go over the index

  if (materialData && materialData->size() > 1
      && materialData->size() < maxItems) {
    postStatusMsg(OSP_LOG_INFO)
        << toString()
        << " potentially not enough 'material' elements for "
           "geometry, clamping";
  }

  if (colorData && colorData->size() > 1 && colorData->size() < maxItems) {
    postStatusMsg(OSP_LOG_INFO)
        << toString()
        << " potentially not enough 'color' elements for geometry, clamping";
  }

  invertNormals = getParam<bool>("invertNormals");

  ispc::GeometricModel_set(getIE(),
      geometry().getIE(),
      ispc(colorData),
      ispc(indexData),
      ispc(materialData),
      useRendererMaterialList,
      invertNormals);
}

OSPTYPEFOR_DEFINITION(GeometricModel *);

} // namespace ospray
