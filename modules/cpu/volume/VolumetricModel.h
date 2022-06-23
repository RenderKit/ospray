// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Volume.h"
#include "common/StructShared.h"
#include "openvkl/openvkl.h"

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

  void setGeomID(int geomID)
  {
    volume->setGeomID(geomID);
  }

 private:
  box3f volumeBounds;
  Ref<Volume> volume;
  const Ref<Volume> volumeAPI;
  VKLIntervalIteratorContext vklIntervalContext{nullptr};
};

OSPTYPEFOR_SPECIALIZATION(VolumetricModel *, OSP_VOLUMETRIC_MODEL);

} // namespace ospray
