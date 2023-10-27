// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
// ospray
#include "ISPCDeviceObject.h"
#include "common/Data.h"
#include "common/FeatureFlagsEnum.h"
#include "fb/ImageOp.h"
#include "ospray/ospray.h"
#include "rkcommon/utility/ArrayView.h"
// ispc shared
#include "FrameBufferShared.h"

namespace ospray {

struct Camera;
struct FrameBufferView;

// abstract frame buffer class
struct OSPRAY_SDK_INTERFACE FrameBuffer
    : public AddStructShared<ISPCDeviceObject, ispc::FrameBuffer>
{
  // app-mappable format of the color buffer. make sure that this
  // matches the definition on the ISPC side
  using ColorBufferFormat = OSPFrameBufferFormat;

  FrameBuffer(api::ISPCDevice &device,
      const vec2i &size,
      ColorBufferFormat colorBufferFormat,
      const uint32 channels,
      const FeatureFlagsOther ffo);

  virtual ~FrameBuffer() override = default;

  virtual void commit() override;

  virtual const void *mapBuffer(OSPFrameBufferChannel channel) = 0;

  virtual void unmap(const void *mappedMem) = 0;

  // clear (the specified channels of) this frame buffer
  virtual void clear();

  // Get number of pixels per render task, in x and y direction
  vec2i getRenderTaskSize() const;

  // Return the number of render tasks in the x and y direction
  // This is the kernel launch dims to render the image
  virtual vec2i getNumRenderTasks() const = 0;

  virtual uint32_t getTotalRenderTasks() const = 0;

  // Get the device-side render task IDs
  virtual utility::ArrayView<uint32_t> getRenderTaskIDs(
      float errorThreshold) = 0;

  // get number of pixels in x and y diretion
  vec2i getNumPixels() const;

  // get the color format type for this Buffer
  ColorBufferFormat getColorBufferFormat() const;

  float getVariance() const;

  virtual float taskError(const uint32_t taskID) const = 0;

  virtual void beginFrame();

  // end the frame and run any final post-processing frame ops
  virtual void endFrame(const float errorThreshold) = 0;

  // Invoke post-processing by calling all FrameOps
  virtual AsyncEvent postProcess(bool wait) = 0;

  // common function to help printf-debugging, every derived class should
  // override this
  virtual std::string toString() const override;

  void setCompletedEvent(OSPSyncEvent event);
  OSPSyncEvent getLatestCompleteEvent() const;
  void waitForEvent(OSPSyncEvent event) const;

  virtual float getCurrentProgress() const;

  virtual void cancelFrame();
  bool frameCancelled() const;

  bool hasColorBuf() const;
  bool hasVarianceBuf() const;
  bool hasNormalBuf() const;
  bool hasAlbedoBuf() const;
  bool hasPrimitiveIDBuf() const;
  bool hasObjectIDBuf() const;
  bool hasInstanceIDBuf() const;

  bool doAccumulation() const;

  uint32 getChannelFlags() const;

  int32_t getFrameID() const;
  void setFrameID(int32_t id);

  FeatureFlags getFeatureFlags() const;

 protected:
  const vec2i size;
  ColorBufferFormat colorBufferFormat;

  // Frame buffer optional buffers
  bool hasColorBuffer;
  bool hasDepthBuffer;
  bool hasVarianceBuffer;
  bool hasNormalBuffer;
  bool hasAlbedoBuffer;
  bool hasPrimitiveIDBuffer;
  bool hasObjectIDBuffer;
  bool hasInstanceIDBuffer;

  // indicates whether the app requested this frame buffer to do accumulation
  bool doAccum;

  uint32_t skipVarianceCounter{1};
  uint32_t skipVarianceFrameCounter{1};
  float frameVariance{0.f};

  std::atomic<bool> cancelRender{false};

  std::atomic<OSPSyncEvent> stagesCompleted{OSP_FRAME_FINISHED};

  Ref<const DataT<ImageOp *>> imageOpData;

  FeatureFlagsOther featureFlags{FFO_NONE};
};

OSPTYPEFOR_SPECIALIZATION(FrameBuffer *, OSP_FRAMEBUFFER);

inline vec2i FrameBuffer::getNumPixels() const
{
  return size;
}

inline FrameBuffer::ColorBufferFormat FrameBuffer::getColorBufferFormat() const
{
  return colorBufferFormat;
}

inline bool FrameBuffer::hasColorBuf() const
{
  return hasColorBuffer;
}

inline bool FrameBuffer::hasVarianceBuf() const
{
  return hasVarianceBuffer;
}

inline bool FrameBuffer::hasNormalBuf() const
{
  return hasNormalBuffer;
}

inline bool FrameBuffer::hasAlbedoBuf() const
{
  return hasAlbedoBuffer;
}

inline bool FrameBuffer::hasPrimitiveIDBuf() const
{
  return hasPrimitiveIDBuffer;
}

inline bool FrameBuffer::hasObjectIDBuf() const
{
  return hasObjectIDBuffer;
}

inline bool FrameBuffer::hasInstanceIDBuf() const
{
  return hasInstanceIDBuffer;
}

inline bool FrameBuffer::doAccumulation() const
{
  return doAccum;
}

inline int32_t FrameBuffer::getFrameID() const
{
  return getSh()->frameID;
}

inline void FrameBuffer::setFrameID(int32_t id)
{
  getSh()->frameID = id;
}

inline FeatureFlags FrameBuffer::getFeatureFlags() const
{
  FeatureFlags ff;
  ff.other = featureFlags;
  return ff;
}

} // namespace ospray
