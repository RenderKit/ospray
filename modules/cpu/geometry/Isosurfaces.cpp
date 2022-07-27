// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Isosurfaces.h"
#include "common/Data.h"
// openvkl
#include "openvkl/openvkl.h"
// ispc-generated files
#include "geometry/Isosurfaces_ispc.h"

namespace ospray {

Isosurfaces::Isosurfaces()
{
  getSh()->super.postIntersect = ispc::Isosurfaces_postIntersect_addr();
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

  createEmbreeUserGeometry((RTCBoundsFunction)&ispc::Isosurfaces_bounds,
      (RTCIntersectFunctionN)&ispc::Isosurfaces_intersect,
      (RTCOccludedFunctionN)&ispc::Isosurfaces_occluded);
  getSh()->isovalues = isovaluesData->data();
  getSh()->volumetricModel = model ? model->getSh() : nullptr;
  getSh()->volume = volume ? volume->getSh() : nullptr;
  getSh()->super.numPrimitives = numPrimitives();
  getSh()->vklHitContext = vklHitContext;

  postCreationInfo();
}

size_t Isosurfaces::numPrimitives() const
{
  return isovaluesData->size();
}

} // namespace ospray
