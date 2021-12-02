// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Volume.h"
#include "common/Data.h"

#include "openvkl/openvkl.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE VolumetricModel : public ManagedObject
{
  VolumetricModel(Volume *geometry);
  ~VolumetricModel() override;
  std::string toString() const override;

  void commit() override;

  RTCGeometry embreeGeometryHandle() const;

  box3f bounds() const;

  Ref<Volume> getVolume() const;

  void setGeomID(int geomID);

 private:
  box3f volumeBounds;
  Ref<Volume> volume;
  Ref<Volume> volumeAPI;
  VKLIntervalIteratorContext vklIntervalContext{nullptr};
};

OSPTYPEFOR_SPECIALIZATION(VolumetricModel *, OSP_VOLUMETRIC_MODEL);

} // namespace ospray
