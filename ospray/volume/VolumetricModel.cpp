// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "VolumetricModel.h"
#include "transferFunction/TransferFunction.h"
// openvkl
#include "openvkl/openvkl.h"
// ispc exports
#include "Volume_ispc.h"
#include "VolumetricModel_ispc.h"

namespace ospray {

VolumetricModel::VolumetricModel(Volume *_volume)
{
  managedObjectType = OSP_VOLUMETRIC_MODEL;

  if (_volume == nullptr)
    throw std::runtime_error("volumetric model received null volume");

  volume = _volume;

  this->ispcEquivalent = ispc::VolumetricModel_create(this, volume->getIE());
}

std::string VolumetricModel::toString() const
{
  return "ospray::VolumetricModel";
}

void VolumetricModel::commit()
{
  auto *transferFunction =
      (TransferFunction *)getParamObject("transferFunction", nullptr);

  if (transferFunction == nullptr)
    throw std::runtime_error("volumetric model must have 'transferFunction'");

  // create value selector using transfer function and pass to volume
  if (volume->vklVolume) {
    if (vklValueSelector) {
      vklRelease(vklValueSelector);
      vklValueSelector = nullptr;
    }

    vklValueSelector = vklNewValueSelector(volume->vklVolume);
    std::vector<range1f> valueRanges =
        transferFunction->getPositiveOpacityValueRanges();
    vklValueSelectorSetRanges(vklValueSelector,
        valueRanges.size(),
        (const vkl_range1f *)valueRanges.data());
    vklCommit(vklValueSelector);
    ispc::VolumetricModel_set_valueSelector(ispcEquivalent, vklValueSelector);
  }

  // Finish getting/setting other appearance information //
  volumeBounds = volume->bounds;

  ispc::VolumetricModel_set(ispcEquivalent,
      transferFunction->getIE(),
      (const ispc::box3f &)volumeBounds,
      getParam<float>("densityScale", 1.f),
      getParam<float>("anisotropy", 0.f));
}

RTCGeometry VolumetricModel::embreeGeometryHandle() const
{
  return volume->embreeGeometry;
}

box3f VolumetricModel::bounds() const
{
  return volumeBounds;
}

Ref<Volume> VolumetricModel::getVolume() const
{
  return volume;
}

void VolumetricModel::setGeomID(int geomID)
{
  ispc::Volume_set_geomID(volume->getIE(), geomID);
}

OSPTYPEFOR_DEFINITION(VolumetricModel *);

} // namespace ospray
