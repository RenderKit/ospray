// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#pragma once

#include "Volume.h"
#include "common/FeatureFlagsEnum.h"
#include "openvkl/openvkl.h"
// comment break to prevent clang-format from reordering openvkl includes
#if OPENVKL_VERSION_MAJOR > 1
#include "openvkl/device/openvkl.h"
#endif
// ispc shared
#include "volume/VolumetricModelShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE VolumetricModel
    : public AddStructShared<ISPCDeviceObject, ispc::VolumetricModel>
{
  VolumetricModel(api::ISPCDevice &device, Volume *geometry);
  ~VolumetricModel() override;
  std::string toString() const override;

  void commit() override;

  RTCGeometry embreeGeometryHandle() const;

  box3f bounds() const;

  Ref<Volume> getVolume() const;

  FeatureFlags getFeatureFlags() const;

 private:
  box3f volumeBounds;
  Ref<Volume> volume;
  const Ref<Volume> volumeAPI;
  VKLIntervalIteratorContext vklIntervalContext = VKLIntervalIteratorContext();
};

OSPTYPEFOR_SPECIALIZATION(VolumetricModel *, OSP_VOLUMETRIC_MODEL);

inline FeatureFlags VolumetricModel::getFeatureFlags() const
{
  return volume->getFeatureFlags();
}

} // namespace ospray
#endif
