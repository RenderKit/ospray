// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <thread>
#include "MultiDeviceObject.h"

// ospray
#include "common/Future.h"
#include "fb/FrameBuffer.h"

namespace ospray {
namespace api {

struct MultiDeviceRenderTask : public Future
{
  MultiDeviceRenderTask(MultiDeviceObject *fb, std::function<float()> fcn);
  ~MultiDeviceRenderTask() override;

  bool isFinished(OSPSyncEvent event = OSP_TASK_FINISHED) override;

  void wait(OSPSyncEvent event) override;
  void cancel() override;
  float getProgress() override;

  float getTaskDuration() override;

 private:
  Ref<MultiDeviceObject> fb;
  std::atomic<float> taskDuration{0.f};
  std::atomic<bool> finished;
  std::thread thread;
};

// Inlined definitions //////////////////////////////////////////////////////

inline MultiDeviceRenderTask::MultiDeviceRenderTask(
    MultiDeviceObject *_fb, std::function<float()> fcn)
    : fb(_fb), finished(false)
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
  FrameBuffer *fb0 = (FrameBuffer *)fb->objects[0];
  return finished == true || fb0->getLatestCompleteEvent() >= event;
}

inline void MultiDeviceRenderTask::wait(OSPSyncEvent event)
{
  if (event == OSP_TASK_FINISHED) {
    while (!finished) {
      std::this_thread::yield();
    }
  } else {
    for (size_t i = 0; i < fb->objects.size(); ++i) {
      FrameBuffer *fbi = (FrameBuffer *)fb->objects[i];
      fbi->waitForEvent(event);
    }
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
  float progress = 1.0;
  for (size_t i = 0; i < fb->objects.size(); ++i) {
    FrameBuffer *fbi = (FrameBuffer *)fb->objects[i];
    float pn = fbi->getCurrentProgress();
    if (pn < progress) {
      progress = pn;
    }
  }
  return progress;
}

inline float MultiDeviceRenderTask::getTaskDuration()
{
    return taskDuration.load();
}

} // namespace api
} // namespace ospray
