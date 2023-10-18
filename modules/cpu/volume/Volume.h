// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#pragma once

#include "ISPCDeviceObject.h"
#include "common/FeatureFlagsEnum.h"
#include "common/StructShared.h"
// embree
#include "common/Embree.h"
// openvkl
#include "openvkl/openvkl.h"
// comment break to prevent clang-format from reordering openvkl includes
#include "openvkl/device/openvkl.h"
// ispc shared
#include "VolumeShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Volume
    : public AddStructShared<ISPCDeviceObject, ispc::Volume>
{
  Volume(api::ISPCDevice &device, const std::string &vklType);
  ~Volume() override;

  std::string toString() const override;
  void commit() override;

  FeatureFlags getFeatureFlags() const;

 private:
  void checkDataStride(const Data *) const;
  void handleParams();

  // Friends //

  friend struct Isosurfaces;
  friend struct VolumetricModel;

  // Data //
  RTCGeometry embreeGeometry{nullptr};
  VKLVolume vklVolume = VKLVolume();
  VKLSampler vklSampler = VKLSampler();

  box3f bounds{empty};

  std::string vklType;

  VKLFeatureFlags vklFeatureFlags = VKL_FEATURE_FLAGS_NONE;
};

OSPTYPEFOR_SPECIALIZATION(Volume *, OSP_VOLUME);

inline FeatureFlags Volume::getFeatureFlags() const
{
  FeatureFlags ff;
  ff.volume = vklFeatureFlags;
  return ff;
}

} // namespace ospray
#endif
