// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#pragma once

#include "ISPCDeviceObject.h"
#include "common/StructShared.h"
#include "common/FeatureFlagsEnum.h"
// embree
#include "common/Embree.h"
// openvkl
#include "openvkl/openvkl.h"
// comment break to prevent clang-format from reordering openvkl includes
#if OPENVKL_VERSION_MAJOR > 1
#include "openvkl/device/openvkl.h"
#endif
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

  FeatureFlagsVolume getFeatureFlagsVolume() const;

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

  FeatureFlagsVolume featureFlags;
};

OSPTYPEFOR_SPECIALIZATION(Volume *, OSP_VOLUME);

inline FeatureFlagsVolume Volume::getFeatureFlagsVolume() const
{
  return featureFlags;
}

} // namespace ospray
#endif
