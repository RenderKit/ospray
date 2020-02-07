// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "api/ISPCDevice.h"
#include "common/Managed.h"
// embree
#include "embree3/rtcore.h"

#include "openvkl/volume.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Volume : public ManagedObject
{
  Volume(const std::string &vklType);
  ~Volume() override;

  std::string toString() const override;

  void commit() override;

 private:
  void handleParams();

  void createEmbreeGeometry();

  // Friends //

  friend struct Isosurfaces;
  friend struct VolumetricModel;

  // Data //

  RTCGeometry embreeGeometry{nullptr};
  VKLVolume vklVolume{nullptr};

  box3f bounds{empty};

  std::string vklType;
};

OSPTYPEFOR_SPECIALIZATION(Volume *, OSP_VOLUME);

} // namespace ospray
