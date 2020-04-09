// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Volume.h"
#include "common/Data.h"

#include "openvkl/value_selector.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE VolumetricModel : public ManagedObject
{
  VolumetricModel(Volume *geometry);
  ~VolumetricModel() override = default;
  std::string toString() const override;

  void commit() override;

  RTCGeometry embreeGeometryHandle() const;

  box3f bounds() const;

  Ref<Volume> getVolume() const;

  void setGeomID(int geomID);

 private:
  box3f volumeBounds;
  Ref<Volume> volume;
  VKLValueSelector vklValueSelector{nullptr};
};

OSPTYPEFOR_SPECIALIZATION(VolumetricModel *, OSP_VOLUMETRIC_MODEL);

} // namespace ospray
