// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "Data.h"
#include "ISPCDeviceObject.h"
// stl
#include <vector>
// embree
#include "embree3/rtcore.h"
// ispc shared
#include "GroupShared.h"

namespace ospray {

struct GeometricModel;
struct VolumetricModel;
struct Light;

struct OSPRAY_SDK_INTERFACE Group
    : public AddStructShared<ISPCDeviceObject, ispc::Group>
{
  Group(api::ISPCDevice &device);
  ~Group() override;

  std::string toString() const override;
  void commit() override;

  box3f getBounds() const override;

  // Data members //

  Ref<const DataT<GeometricModel *>> geometricModels;
  Ref<const DataT<VolumetricModel *>> volumetricModels;
  Ref<const DataT<GeometricModel *>> clipModels;
  int numInvertedClippers{0};

  Ref<const DataT<Light *>> lights;

  RTCScene sceneGeometries{nullptr};
  RTCScene sceneVolumes{nullptr};
  RTCScene sceneClippers{nullptr};

 private:
  std::unique_ptr<BufferShared<ispc::GeometricModel *>> geometricModelsArray;
  std::unique_ptr<BufferShared<ispc::VolumetricModel *>> volumetricModelsArray;
  std::unique_ptr<BufferShared<ispc::GeometricModel *>> clipModelsArray;
};

OSPTYPEFOR_SPECIALIZATION(Group *, OSP_GROUP);

} // namespace ospray
