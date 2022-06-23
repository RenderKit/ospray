// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDevice.h"
#include "common/Managed.h"
#include "common/StructShared.h"
// embree
#include "embree3/rtcore.h"

#include "openvkl/volume.h"
#include "volume/VolumeShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Volume
    : public AddStructShared<ManagedObject, ispc::Volume>
{
  Volume(const std::string &vklType);
  ~Volume() override;

  std::string toString() const override;
  void commit() override;
  void setDevice(RTCDevice embreeDevice, VKLDevice vklDevice);
  void setGeomID(int geomID);

 private:
  void checkDataStride(const Data *) const;
  void handleParams();

  // Friends //

  friend struct Isosurfaces;
  friend struct VolumetricModel;

  // Data //
  RTCGeometry embreeGeometry{nullptr};
  VKLVolume vklVolume{nullptr};
  VKLSampler vklSampler{nullptr};

  box3f bounds{empty};

  std::string vklType;
  RTCDevice embreeDevice{nullptr};
  VKLDevice vklDevice{nullptr};
};

OSPTYPEFOR_SPECIALIZATION(Volume *, OSP_VOLUME);

} // namespace ospray
