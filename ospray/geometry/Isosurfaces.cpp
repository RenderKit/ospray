// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Isosurfaces.h"
#include "common/Data.h"
#include "common/World.h"
// openvkl
#include "openvkl/openvkl.h"
// ispc-generated files
#include "Isosurfaces_ispc.h"

namespace ospray {

Isosurfaces::~Isosurfaces()
{
  if (valueSelector) {
    vklRelease(valueSelector);
    valueSelector = nullptr;
  }
}

std::string Isosurfaces::toString() const
{
  return "ospray::Isosurfaces";
}

void Isosurfaces::commit()
{
  isovaluesData = getParamDataT<float>("isovalue", true, true);

  model = (VolumetricModel *)getParamObject("volume");

  if (!model) {
    throw std::runtime_error(
        "the 'volume' parameter must be set for isosurfaces");
  }

  if (!isovaluesData->compact()) {
    // get rid of stride
    auto data = new Data(OSP_FLOAT, vec3ui(isovaluesData->size(), 1, 1));
    data->copy(*isovaluesData, vec3ui(0));
    isovaluesData = &(data->as<float>());
    data->refDec();
  }

  if (valueSelector) {
    vklRelease(valueSelector);
    valueSelector = nullptr;
  }

  valueSelector = vklNewValueSelector(model->getVolume()->vklVolume);

  if (isovaluesData->size() > 0) {
    vklValueSelectorSetValues(
        valueSelector, isovaluesData->size(), isovaluesData->data());
  }

  vklCommit(valueSelector);

  postCreationInfo();
}

LiveGeometry Isosurfaces::createEmbreeGeometry()
{
  LiveGeometry retval;

  retval.ispcEquivalent = ispc::Isosurfaces_create(this);
  retval.embreeGeometry =
      rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);

  ispc::Isosurfaces_set(retval.ispcEquivalent,
      retval.embreeGeometry,
      isovaluesData->size(),
      isovaluesData->data(),
      model->getIE(),
      valueSelector);

  return retval;
}

size_t Isosurfaces::numPrimitives() const
{
  return isovaluesData->size();
}

OSP_REGISTER_GEOMETRY(Isosurfaces, isosurface);

} // namespace ospray
