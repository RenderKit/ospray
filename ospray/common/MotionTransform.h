// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "./Group.h"
// embree
#include "embree3/rtcore.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE MotionTransform : public ManagedObject
{
  void commit() override;

  Ref<const DataT<affine3f>> motionTransforms;
  range1f time{0.0f, 1.0f};
  bool motionBlur{false};
};

} // namespace ospray
