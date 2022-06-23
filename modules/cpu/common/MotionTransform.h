// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "./Group.h"
// embree
#include "embree3/rtcore.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE MotionTransform
{
  void readParams(ManagedObject &);
  void setEmbreeTransform(RTCGeometry) const;
  affine3f transform{one};
  bool motionBlur{false};
  bool quaternion{false};

 private:
  Ref<const DataT<affine3f>> transforms;
  std::vector<RTCQuaternionDecomposition> quaternions;
  range1f time{0.0f, 1.0f};
};

} // namespace ospray
