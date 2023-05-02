// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "common/Future.h"

namespace ospray {

struct RenderTask : public Future
{
  RenderTask(sycl::event rendererEvent, sycl::event frameBufferEvent);
  ~RenderTask() override;

  bool isFinished(OSPSyncEvent event = OSP_TASK_FINISHED) override;
  void wait(OSPSyncEvent event = OSP_TASK_FINISHED) override;
  void cancel() override;
  float getProgress() override;
  float getTaskDuration() override;

 private:
  sycl::event rendererEvent;
  sycl::event frameBufferEvent;
};

// Inlined definitions //////////////////////////////////////////////////////

inline RenderTask::RenderTask(
    sycl::event rendererEvent, sycl::event frameBufferEvent)
    : rendererEvent(rendererEvent), frameBufferEvent(frameBufferEvent)
{}

inline RenderTask::~RenderTask()
{
  // Mimic non-sycl RenderTask behavior which waits on destruction
  wait();
}

inline bool RenderTask::isFinished(OSPSyncEvent event)
{
  switch (event) {
  case OSP_TASK_FINISHED:
  case OSP_FRAME_FINISHED:
    frameBufferEvent.wait_and_throw();
    [[fallthrough]];
  case OSP_WORLD_RENDERED:
    rendererEvent.wait_and_throw();
    [[fallthrough]];
  default:
    return true;
  }

  // The proper way of checking is commented out because it degrades
  // performance by a factor of 3. Analysis shows that sharing GPU
  // between SYCL kernel and OpenGL Draws/SwapBuffers is highly ineffective.
  // Need to first finish SYCL kernel then render FB and GUI.
  // return (syclEvent.get_info<sycl::info::event::command_execution_status>()
  //     == sycl::info::event_command_status::complete);
}

inline void RenderTask::wait(OSPSyncEvent event)
{
  switch (event) {
  case OSP_TASK_FINISHED:
  case OSP_FRAME_FINISHED:
    frameBufferEvent.wait_and_throw();
    [[fallthrough]];
  case OSP_WORLD_RENDERED:
    rendererEvent.wait_and_throw();
    [[fallthrough]];
  default:
    return;
  }
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
      rendererEvent
          .get_profiling_info<sycl::info::event_profiling::command_start>();
  const auto t1 =
      rendererEvent
          .get_profiling_info<sycl::info::event_profiling::command_end>();
  // TODO: We need a way to tell if the fbEvent was actually submitted,
  // otherwise getting the time gives an error
  /*
  const auto t2 =
      frameBufferEvent
          .get_profiling_info<sycl::info::event_profiling::command_start>();
  const auto t3 =
      frameBufferEvent
          .get_profiling_info<sycl::info::event_profiling::command_end>();
  return ((t1 - t0) + (t3 - t2)) * 1E-9;
          */
  return (t1 - t0) * 1.0e-9;
}

} // namespace ospray
