// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "../geometry/GeometricModel.h"
#include "../volume/VolumetricModel.h"
#include "./Data.h"
#include "./Managed.h"
// stl
#include <vector>
// embree
#include "embree3/rtcore.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Group : public ManagedObject
{
  Group();
  ~Group() override;

  std::string toString() const override;
  void commit() override;

  box3f getBounds() const override;

  // Data members //

  Ref<const DataT<GeometricModel *>> geometricModels;
  std::vector<void *> geometryIEs;
  std::vector<void *> geometricModelIEs;

  Ref<const DataT<VolumetricModel *>> volumetricModels;
  std::vector<void *> volumeIEs;
  std::vector<void *> volumetricModelIEs;

  Ref<const DataT<GeometricModel *>> clipModels;
  std::vector<void *> clipIEs;
  std::vector<void *> clipModelIEs;
  int numInvertedClippers{0};

  RTCScene sceneGeometries{nullptr};
  RTCScene sceneVolumes{nullptr};
  RTCScene sceneClippers{nullptr};
};

OSPTYPEFOR_SPECIALIZATION(Group *, OSP_GROUP);

} // namespace ospray
