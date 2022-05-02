// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "Data.h"
#include "Managed.h"
// stl
#include <vector>
// embree
#include "embree3/rtcore.h"
// ispc shared
#include "WorldShared.h"

namespace ospray {

struct Instance;
struct Light;

struct OSPRAY_SDK_INTERFACE World
    : public AddStructShared<ManagedObject, ispc::World>
{
  World();
  virtual ~World() override;

  std::string toString() const override;
  void commit() override;

  box3f getBounds() const override;

  // Data members //

  Ref<const DataT<Instance *>> instances;
  Ref<const DataT<Light *>> lights;

  bool scivisDataValid{false};
  bool pathtracerDataValid{false};

  void setDevice(RTCDevice embreeDevice);

 protected:
  RTCDevice embreeDevice{nullptr};
};

OSPTYPEFOR_SPECIALIZATION(World *, OSP_WORLD);

} // namespace ospray
