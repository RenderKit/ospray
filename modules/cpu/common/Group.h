// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "Data.h"
#include "FeatureFlagsEnum.h"
#include "ISPCDeviceObject.h"
#include "StructShared.h"
// stl
#include <vector>
// ispc shared
#include "GroupShared.h"

namespace ospray {

struct GeometricModel;
#ifdef OSPRAY_ENABLE_VOLUMES
struct VolumetricModel;
#endif
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
#ifdef OSPRAY_ENABLE_VOLUMES
  Ref<const DataT<VolumetricModel *>> volumetricModels;
#endif
  Ref<const DataT<GeometricModel *>> clipModels;
  int numInvertedClippers{0};

  Ref<const DataT<Light *>> lights;

  RTCScene sceneGeometries{nullptr};
#ifdef OSPRAY_ENABLE_VOLUMES
  RTCScene sceneVolumes{nullptr};
#endif
  RTCScene sceneClippers{nullptr};

  const FeatureFlags &getFeatureFlags() const;

 private:
  FeatureFlags featureFlags;

  std::unique_ptr<BufferShared<ispc::GeometricModel *>> geometricModelsArray;
#ifdef OSPRAY_ENABLE_VOLUMES
  std::unique_ptr<BufferShared<ispc::VolumetricModel *>> volumetricModelsArray;
#endif
  std::unique_ptr<BufferShared<ispc::GeometricModel *>> clipModelsArray;
};

OSPTYPEFOR_SPECIALIZATION(Group *, OSP_GROUP);

inline const FeatureFlags &Group::getFeatureFlags() const
{
  return featureFlags;
}

} // namespace ospray
