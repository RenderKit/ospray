// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospcommon
#include "ospcommon/tasking/AsyncTask.h"
// ospray
#include "../common/Future.h"
#include "../fb/FrameBuffer.h"

namespace ospray {

struct RenderTask : public Future, public tasking::AsyncTask<float>
{
  RenderTask(FrameBuffer *, std::function<float()> fcn);
  ~RenderTask() override = default;

  bool isFinished(OSPSyncEvent event = OSP_TASK_FINISHED) override;

  void wait(OSPSyncEvent event) override;
  void cancel() override;
  float getProgress() override;

 private:
  Ref<FrameBuffer> fb;
};

// Inlined definitions //////////////////////////////////////////////////////

inline RenderTask::RenderTask(FrameBuffer *_fb, std::function<float()> fcn)
    : AsyncTask<float>(fcn), fb(_fb)
{}

inline bool RenderTask::isFinished(OSPSyncEvent event)
{
  return finished() || fb->getLatestCompleteEvent() >= event;
}

inline void RenderTask::wait(OSPSyncEvent event)
{
  if (event == OSP_TASK_FINISHED)
    AsyncTask<float>::wait();
  else
    fb->waitForEvent(event);
}

inline void RenderTask::cancel()
{
  fb->cancelFrame();
}

inline float RenderTask::getProgress()
{
  return fb->getCurrentProgress();
}

} // namespace ospray
