// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

// ospray
#include "Isosurfaces.h"
#include "common/Data.h"
// openvkl
#include "openvkl/openvkl.h"
// comment break to prevent clang-format from reordering openvkl includes
#include "openvkl/device/openvkl.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc-generated files
#include "geometry/Isosurfaces_ispc.h"
#else
namespace ispc {
void Isosurfaces_bounds(const RTCBoundsFunctionArguments *uniform args);
}
#endif

namespace ospray {

Isosurfaces::Isosurfaces(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device, FFG_ISOSURFACE)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.postIntersect = ispc::Isosurfaces_postIntersect_addr();
#endif
}

Isosurfaces::~Isosurfaces()
{
  if (vklHitContext) {
    vklRelease(vklHitContext);
  }
}

std::string Isosurfaces::toString() const
{
  return "ospray::Isosurfaces";
}

void Isosurfaces::commit()
{
  isovaluesData = getParamDataT<float>("isovalue", true, true);

  volume = getParamObject<Volume>("volume");
  if (!volume)
    throw std::runtime_error(toString() + "must have 'volume'");

  if (!isovaluesData->compact()) {
    // get rid of stride
    auto data = new Data(
        getISPCDevice(), OSP_FLOAT, vec3ui(isovaluesData->size(), 1, 1));
    data->copy(*isovaluesData, vec3ui(0));
    isovaluesData = &(data->as<float>());
    data->refDec();
  }

  if (vklHitContext) {
    vklRelease(vklHitContext);
  }

  vklHitContext = vklNewHitIteratorContext(volume->vklSampler);

  if (isovaluesData->size() > 0) {
    VKLData valuesData = vklNewData(getISPCDevice().getVklDevice(),
        isovaluesData->size(),
        VKL_FLOAT,
        isovaluesData->data());
    vklSetData(vklHitContext, "values", valuesData);
    vklRelease(valuesData);
  }

  vklCommit(vklHitContext);

  createEmbreeUserGeometry((RTCBoundsFunction)&ispc::Isosurfaces_bounds);
  getSh()->isovalues = isovaluesData->data();
  getSh()->volume = volume->getSh();
  getSh()->super.numPrimitives = numPrimitives();
  getSh()->vklHitContext = vklHitContext;

  postCreationInfo();
}

size_t Isosurfaces::numPrimitives() const
{
  return isovaluesData->size();
}

} // namespace ospray

#endif
