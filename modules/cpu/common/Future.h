// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "./Managed.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Future : public ManagedObject
{
  Future();
  ~Future() override = default;

  std::string toString() const override;

  virtual bool isFinished(OSPSyncEvent = OSP_TASK_FINISHED) = 0;

  virtual void wait(OSPSyncEvent = OSP_TASK_FINISHED) = 0;
  virtual void cancel() = 0;
  virtual float getProgress() = 0;

  virtual float getTaskDuration() = 0;
};

OSPTYPEFOR_SPECIALIZATION(Future *, OSP_FUTURE);

} // namespace ospray
