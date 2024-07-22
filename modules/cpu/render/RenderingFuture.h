// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "common/DeviceRT.h"
#include "common/Future.h"
#include "fb/FrameBuffer.h"

namespace ospray {

struct RenderingFuture : public Future
{
  RenderingFuture(FrameBuffer *fb,
      devicert::AsyncEvent rendererEvent,
      devicert::AsyncEvent frameBufferEvent,
      devicert::AsyncEvent finalHostEvent);
  ~RenderingFuture() override;

  bool isFinished(OSPSyncEvent event = OSP_TASK_FINISHED) override;
  void wait(OSPSyncEvent event = OSP_TASK_FINISHED) override;
  void cancel() override;
  float getProgress() override;
  float getTaskDuration() override;

 private:
  Ref<FrameBuffer> fb;
  devicert::AsyncEvent rendererEvent;
  devicert::AsyncEvent frameBufferEvent;
  devicert::AsyncEvent finalHostEvent;
};

// Inlined definitions //////////////////////////////////////////////////////

inline RenderingFuture::RenderingFuture(FrameBuffer *fb,
    devicert::AsyncEvent rendererEvent,
    devicert::AsyncEvent frameBufferEvent,
    devicert::AsyncEvent finalHostEvent)
    : fb(fb),
      rendererEvent(rendererEvent),
      frameBufferEvent(frameBufferEvent),
      finalHostEvent(finalHostEvent)
{}

inline RenderingFuture::~RenderingFuture()
{
  // Wait for all work to be done when OSPRay future is released. This behavior
  // is mainly for OSPRay backwards compatibility,
  rendererEvent.wait();
  frameBufferEvent.wait();
  finalHostEvent.wait();
}

inline bool RenderingFuture::isFinished(OSPSyncEvent event)
{
  switch (event) {
  case OSP_TASK_FINISHED:
    return finalHostEvent.finished();
  case OSP_FRAME_FINISHED:
    return frameBufferEvent.finished();
  case OSP_WORLD_RENDERED:
    return rendererEvent.finished();
  default:
    return true;
  }
}

inline void RenderingFuture::wait(OSPSyncEvent event)
{
  switch (event) {
  case OSP_TASK_FINISHED:
    rendererEvent.wait();
    frameBufferEvent.wait();
    finalHostEvent.wait();
    return;
  case OSP_FRAME_FINISHED:
    rendererEvent.wait();
    frameBufferEvent.wait();
    return;
  case OSP_WORLD_RENDERED:
    rendererEvent.wait();
    return;
  default:
    return;
  }
}

inline void RenderingFuture::cancel()
{
  fb->cancelFrame();
}

inline float RenderingFuture::getProgress()
{
  return fb->getCurrentProgress();
}

inline float RenderingFuture::getTaskDuration()
{
  return rendererEvent.getDuration() + frameBufferEvent.getDuration()
      + finalHostEvent.getDuration();
}

} // namespace ospray
