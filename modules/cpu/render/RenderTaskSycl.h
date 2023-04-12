// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "../common/Future.h"

namespace ospray {

struct RenderTask : public Future
{
  RenderTask(sycl::event);
  ~RenderTask() override = default;

  bool isFinished(OSPSyncEvent event = OSP_TASK_FINISHED) override;
  void wait(OSPSyncEvent event = OSP_TASK_FINISHED) override;
  void cancel() override;
  float getProgress() override;
  float getTaskDuration() override;

 private:
  sycl::event syclEvent;
};

// Inlined definitions //////////////////////////////////////////////////////

inline RenderTask::RenderTask(sycl::event syclEvent) : syclEvent(syclEvent) {}

inline bool RenderTask::isFinished(OSPSyncEvent event)
{
  (void)event;
  syclEvent.wait_and_throw();
  return true;

  // The proper way of checking is commented out because it degrades
  // performance by a factor of 3. Analysis shows that sharing GPU
  // between SYCL kernel and OpenGL Draws/SwapBuffers is highly ineffective.
  // Need to first finish SYCL kernel then render FB and GUI.
  // return (syclEvent.get_info<sycl::info::event::command_execution_status>()
  //     == sycl::info::event_command_status::complete);
}

inline void RenderTask::wait(OSPSyncEvent event)
{
  (void)event;
  syclEvent.wait_and_throw();
}

inline void RenderTask::cancel()
{
  // No task canceling on GPU for now
}

inline float RenderTask::getProgress()
{
  // No progress reported on GPU for now
  return 0.f;
}

inline float RenderTask::getTaskDuration()
{
  const auto t0 =
      syclEvent
          .get_profiling_info<sycl::info::event_profiling::command_start>();
  const auto t1 =
      syclEvent.get_profiling_info<sycl::info::event_profiling::command_end>();
  return (t1 - t0) * 1E-9;
}

} // namespace ospray
