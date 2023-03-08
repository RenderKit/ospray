// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "Group.h"
#include "MotionTransform.h"
// ispc shared
#include "InstanceShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Instance
    : public AddStructShared<ISPCDeviceObject, ispc::Instance>
{
  Instance(api::ISPCDevice &device, Group *group);
  ~Instance() override = default;

  std::string toString() const override;

  void commit() override;

  box3f getBounds() const override;

  void setEmbreeGeom(RTCGeometry geom);

  Ref<Group> group;
  const Ref<Group> groupAPI;
  MotionTransform motionTransform;
};

OSPTYPEFOR_SPECIALIZATION(Instance *, OSP_INSTANCE);

} // namespace ospray
