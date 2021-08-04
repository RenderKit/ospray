// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Isosurfaces.h"
#include "common/Data.h"
#include "common/World.h"
// openvkl
#include "openvkl/openvkl.h"
// ispc-generated files
#include "geometry/Isosurfaces_ispc.h"

namespace ospray {

Isosurfaces::Isosurfaces()
{
  ispcEquivalent = ispc::Isosurfaces_create(this);
}

Isosurfaces::~Isosurfaces()
{
  if (vklHitContext) {
    vklRelease(vklHitContext);
    vklHitContext = nullptr;
  }
}

std::string Isosurfaces::toString() const
{
  return "ospray::Isosurfaces";
}

void Isosurfaces::commit()
{
  if (!embreeDevice) {
    throw std::runtime_error("invalid Embree device");
  }
  if (!embreeGeometry) {
    embreeGeometry = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_USER);
  }
  isovaluesData = getParamDataT<float>("isovalue", true, true);
  model = nullptr;

  volume = dynamic_cast<Volume *>(getParamObject("volume"));
  if (!volume) {
    model = dynamic_cast<VolumetricModel *>(getParamObject("volume"));
    if (!model) {
      throw std::runtime_error(
          "the 'volume' parameter must be set for isosurfaces");
    }
    postStatusMsg(
        "ospray::Isosurfaces deprecated parameter use. Isosurfaces "
        "will begin taking an OSPVolume directly, with appearance set through "
        "the GeometricModel instead.",
        OSP_LOG_DEBUG);
  }

  if (!isovaluesData->compact()) {
    // get rid of stride
    auto data = new Data(OSP_FLOAT, vec3ui(isovaluesData->size(), 1, 1));
    data->copy(*isovaluesData, vec3ui(0));
    isovaluesData = &(data->as<float>());
    data->refDec();
  }

  if (vklHitContext) {
    vklRelease(vklHitContext);
    vklHitContext = nullptr;
  }

  VKLDevice vklDevice = nullptr;
  if (volume) {
    vklHitContext = vklNewHitIteratorContext(volume->vklSampler);
    vklDevice = volume->vklDevice;
  } else {
    vklHitContext = vklNewHitIteratorContext(model->getVolume()->vklSampler);
    vklDevice = model->getVolume()->vklDevice;
  }

  if (isovaluesData->size() > 0) {
    VKLData valuesData = vklNewData(
        vklDevice, isovaluesData->size(), VKL_FLOAT, isovaluesData->data());
    vklSetData(vklHitContext, "values", valuesData);
    vklRelease(valuesData);
  }

  vklCommit(vklHitContext);

  ispc::Isosurfaces_set(getIE(),
      embreeGeometry,
      isovaluesData->size(),
      isovaluesData->data(),
      model ? model->getIE() : nullptr,
      volume ? volume->getIE() : nullptr,
      vklHitContext);

  postCreationInfo();
}

size_t Isosurfaces::numPrimitives() const
{
  return isovaluesData->size();
}

} // namespace ospray
