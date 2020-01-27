// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "./Data.h"
#include "./Managed.h"
#include "Instance.h"
#include "lights/Light.h"
// stl
#include <vector>
// embree
#include "embree3/rtcore.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE World : public ManagedObject
{
  World();
  virtual ~World() override;

  std::string toString() const override;
  void commit() override;

  box3f getBounds() const override;

  // Data members //

  Ref<const DataT<Instance *>> instances;
  Ref<const DataT<Light *>> lights;

  std::vector<void *> instanceIEs;
  int numGeometries{0};
  int numVolumes{0};

  //! \brief the embree scene handle for this geometry
  RTCScene embreeSceneHandleGeometries{nullptr};
  RTCScene embreeSceneHandleVolumes{nullptr};
};

OSPTYPEFOR_SPECIALIZATION(World *, OSP_WORLD);

} // namespace ospray
