// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class Future : public ManagedObject<OSPFuture, OSP_FUTURE>
{
 public:
  Future(const Future &copy);
  Future(OSPFuture existing = nullptr);

  bool isReady(OSPSyncEvent = OSP_TASK_FINISHED);
  void wait(OSPSyncEvent = OSP_TASK_FINISHED);
  void cancel();
  float progress();
  float duration();
};

static_assert(sizeof(Future) == sizeof(OSPFuture),
    "cpp::Future can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Future::Future(const Future &copy)
    : ManagedObject<OSPFuture, OSP_FUTURE>(copy.handle())
{
  ospRetain(copy.handle());
}

inline Future::Future(OSPFuture existing)
    : ManagedObject<OSPFuture, OSP_FUTURE>(existing)
{}

inline bool Future::isReady(OSPSyncEvent e)
{
  return ospIsReady(handle(), e);
}

inline void Future::wait(OSPSyncEvent e)
{
  ospWait(handle(), e);
}

inline void Future::cancel()
{
  ospCancel(handle());
}

inline float Future::progress()
{
  return ospGetProgress(handle());
}

inline float Future::duration()
{
  return ospGetTaskDuration(handle());
}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Future, OSP_FUTURE);

} // namespace ospray
