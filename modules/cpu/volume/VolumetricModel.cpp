// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#include "openvkl/openvkl.h"

// ospray
#include "VolumetricModel.h"

namespace ospray {

VolumetricModel::VolumetricModel(api::ISPCDevice &device, Volume *_volume)
    : AddStructShared(device.getDRTDevice(), device), volumeAPI(_volume)
{
  managedObjectType = OSP_VOLUMETRIC_MODEL;
}

VolumetricModel::~VolumetricModel()
{
  if (vklIntervalContext)
    vklRelease(vklIntervalContext);
}

std::string VolumetricModel::toString() const
{
  return "ospray::VolumetricModel";
}

void VolumetricModel::commit()
{
  volume = getParamObject<Volume>("volume", volumeAPI.ptr);
  if (!volume)
    throw std::runtime_error(toString() + " received NULL 'volume'");

  transferFunction = getParamObject<TransferFunction>("transferFunction");
  if (!transferFunction)
    throw std::runtime_error(toString() + " must have 'transferFunction'");

  // create value selector using transfer function and pass to volume
  if (volume->vklVolume) {
    if (vklIntervalContext) {
      vklRelease(vklIntervalContext);
    }

    vklIntervalContext = vklNewIntervalIteratorContext(volume->vklSampler);
    std::vector<range1f> valueRanges =
        transferFunction->getPositiveOpacityValueRanges();
    if (valueRanges.empty()) {
      // volume made completely transparent and could be removed, which is
      // awkward here
      // set an "empty" interesting value range instead, which will lead to a
      // quick out during volume iteration
      valueRanges.push_back(range1f(neg_inf, neg_inf));
    }
    VKLData valueRangeData = vklNewData(getISPCDevice().getVklDevice(),
        valueRanges.size(),
        VKL_BOX1F,
        valueRanges.data());
    vklSetData(vklIntervalContext, "valueRanges", valueRangeData);
    vklRelease(valueRangeData);
    vklCommit(vklIntervalContext);

    // Pass interval context to ISPC
    getSh()->vklIntervalContext = vklIntervalContext;
  }

  // Finish getting/setting other appearance information
  volumeBounds = volume->bounds;

  // Initialize shared structure
  getSh()->volume = getVolume()->getSh();
  getSh()->transferFunction = transferFunction->getSh();
  getSh()->boundingBox = volumeBounds;
  getSh()->densityScale = getParam<float>("densityScale", 1.f);
  getSh()->anisotropy = getParam<float>("anisotropy", 0.f);
  getSh()->gradientShadingScale = getParam<float>("gradientShadingScale", 0.f);
  getSh()->userID = getParam<uint32>("id", RTC_INVALID_GEOMETRY_ID);

  featureFlagsOther = FFO_VOLUME_IN_SCENE;
  if (getSh()->gradientShadingScale > 0.f)
    featureFlagsOther |= FFO_VOLUME_SCIVIS_SHADING;
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

OSPTYPEFOR_DEFINITION(VolumetricModel *);

} // namespace ospray
#endif
