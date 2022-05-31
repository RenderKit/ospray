// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Volume.h"
#include "openvkl/openvkl.h"
// ispc shared
#include "volume/VolumetricModelShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE VolumetricModel
    : public AddStructShared<ManagedObject, ispc::VolumetricModel>
{
  VolumetricModel(Volume *geometry);
  ~VolumetricModel() override;
  std::string toString() const override;

  void commit() override;

  RTCGeometry embreeGeometryHandle() const;

  box3f bounds() const;

  Ref<Volume> getVolume() const;

 private:
  box3f volumeBounds;
  Ref<Volume> volume;
  const Ref<Volume> volumeAPI;
  VKLIntervalIteratorContext vklIntervalContext{nullptr};
};

OSPTYPEFOR_SPECIALIZATION(VolumetricModel *, OSP_VOLUMETRIC_MODEL);

} // namespace ospray
