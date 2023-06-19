// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "Data.h"
#include "FeatureFlagsEnum.h"
#include "ISPCDeviceObject.h"
// stl
#include <vector>
// ispc shared
#include "WorldShared.h"

namespace ospray {

struct Instance;
struct Light;
struct PathTracerData;
struct SciVisData;

struct OSPRAY_SDK_INTERFACE World
    : public AddStructShared<ISPCDeviceObject, ispc::World>
{
  World(api::ISPCDevice &device);
  virtual ~World() override;

  std::string toString() const override;
  void commit() override;

  box3f getBounds() const override;

  const FeatureFlags &getFeatureFlags() const;

  // Data members //

  Ref<const DataT<Instance *>> instances;
  Ref<const DataT<Light *>> lights;

  std::unique_ptr<BufferShared<ispc::Instance *>> instanceArray;

  std::unique_ptr<SciVisData> scivisData;
  std::unique_ptr<PathTracerData> pathtracerData;

 private:
  FeatureFlags featureFlags;
};

OSPTYPEFOR_SPECIALIZATION(World *, OSP_WORLD);

inline const FeatureFlags &World::getFeatureFlags() const
{
  return featureFlags;
}

} // namespace ospray
