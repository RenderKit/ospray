// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <thread>
#include "MultiDeviceFrameBuffer.h"

// ospray
#include "common/Future.h"
#include "fb/FrameBuffer.h"

namespace ospray {
namespace api {

struct MultiDeviceRenderTask : public Future
{
  MultiDeviceRenderTask(MultiDeviceFrameBuffer *fb, std::function<float()> fcn);
  ~MultiDeviceRenderTask() override;

  bool isFinished(OSPSyncEvent event = OSP_TASK_FINISHED) override;

  void wait(OSPSyncEvent event) override;
  void cancel() override;
  float getProgress() override;

  float getTaskDuration() override;

 private:
  Ref<MultiDeviceFrameBuffer> fb;
  std::atomic<float> taskDuration{0.f};
  std::atomic<bool> finished;
  std::thread thread;
};

// Inlined definitions //////////////////////////////////////////////////////

inline MultiDeviceRenderTask::MultiDeviceRenderTask(
    MultiDeviceFrameBuffer *fb, std::function<float()> fcn)
    : fb(fb), finished(false)
{
  thread = std::thread([this, fcn]() {
    taskDuration = fcn();
    finished = true;
  });
}

inline MultiDeviceRenderTask::~MultiDeviceRenderTask()
{
  if (thread.joinable()) {
    thread.join();
  }
}

inline bool MultiDeviceRenderTask::isFinished(OSPSyncEvent event)
{
  return finished == true || fb->rowmajorFb->getLatestCompleteEvent() >= event;
}

inline void MultiDeviceRenderTask::wait(OSPSyncEvent event)
{
  if (event == OSP_TASK_FINISHED) {
    while (!finished) {
      std::this_thread::yield();
    }
  } else {
    // the rowmajorFb tracks the same events as the subdevices, so we can wait
    // on it
    fb->rowmajorFb->waitForEvent(event);
  }
}

inline void MultiDeviceRenderTask::cancel()
{
  for (size_t i = 0; i < fb->objects.size(); ++i) {
    FrameBuffer *fbi = (FrameBuffer *)fb->objects[i];
    fbi->cancelFrame();
  }
}

inline float MultiDeviceRenderTask::getProgress()
{
  // The frame is done when all the subdevices are done, so the total progress
  // works out to be the average
  float progress = 0.0;
  for (size_t i = 0; i < fb->objects.size(); ++i) {
    FrameBuffer *fbi = (FrameBuffer *)fb->objects[i];
    progress += fbi->getCurrentProgress();
  }
  return progress / fb->objects.size();
}

inline float MultiDeviceRenderTask::getTaskDuration()
{
  return taskDuration.load();
}

} // namespace api
} // namespace ospray
