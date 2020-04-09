// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <thread>
// ospray
#include "common/Future.h"
#include "fb/FrameBuffer.h"

namespace ospray {
namespace mpi {

/* An asynchronous render task which runs on a thread independent
 * of the tasking system's thread pool
 */
struct ThreadedRenderTask : public Future
{
  ThreadedRenderTask(FrameBuffer *, std::function<float()> fcn);
  ~ThreadedRenderTask() override;

  bool isFinished(OSPSyncEvent event = OSP_TASK_FINISHED) override;

  void wait(OSPSyncEvent event) override;
  void cancel() override;
  float getProgress() override;

  float getTaskDuration() override;

 private:
  Ref<FrameBuffer> fb;
  std::atomic<float> taskDuration{0.f};
  std::atomic<bool> finished;
  std::thread thread;
};

// Inlined definitions //////////////////////////////////////////////////////

inline ThreadedRenderTask::ThreadedRenderTask(
    FrameBuffer *_fb, std::function<float()> fcn)
    : fb(_fb), finished(false)
{
  thread = std::thread([this, fcn]() {
    taskDuration = fcn();
    finished = true;
  });
}

inline ThreadedRenderTask::~ThreadedRenderTask()
{
  if (thread.joinable()) {
    thread.join();
  }
}

inline bool ThreadedRenderTask::isFinished(OSPSyncEvent event)
{
  return finished == true || fb->getLatestCompleteEvent() >= event;
}

inline void ThreadedRenderTask::wait(OSPSyncEvent event)
{
  if (event == OSP_TASK_FINISHED) {
    while (!finished) {
      std::this_thread::yield();
    }
  } else
    fb->waitForEvent(event);
}

inline void ThreadedRenderTask::cancel()
{
  fb->cancelFrame();
}

inline float ThreadedRenderTask::getProgress()
{
  return fb->getCurrentProgress();
}

inline float ThreadedRenderTask::getTaskDuration()
{
    return taskDuration.load();
}

} // namespace mpi
} // namespace ospray
