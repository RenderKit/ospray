// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "Group.h"
// embree
#include "Embree.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE MotionTransform
{
  void readParams(ISPCDeviceObject &);
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
