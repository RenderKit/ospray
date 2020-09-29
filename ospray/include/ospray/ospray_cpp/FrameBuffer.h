// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstring>
#include "Camera.h"
#include "Future.h"
#include "GeometricModel.h"
#include "Instance.h"
#include "Renderer.h"
#include "World.h"

namespace ospray {
namespace cpp {

struct PickResult
{
  bool hasHit{false};
  float worldPosition[3];
  Instance instance{(OSPInstance) nullptr};
  GeometricModel model{(OSPGeometricModel) nullptr};
  uint32_t primID{0};
};

class FrameBuffer : public ManagedObject<OSPFrameBuffer, OSP_FRAMEBUFFER>
{
 public:
  // NOTE(jda) - this does *not* create the underlying OSP object
  FrameBuffer() = default;

  FrameBuffer(int size_x,
      int size_y,
      OSPFrameBufferFormat format = OSP_FB_SRGBA,
      int channels = OSP_FB_COLOR);

  FrameBuffer(const FrameBuffer &copy);
  FrameBuffer(OSPFrameBuffer existing);

  FrameBuffer &operator=(const FrameBuffer &copy);

  void resetAccumulation() const;

  Future renderFrame(
      const Renderer &renderer, const Camera &camera, const World &world) const;

  PickResult pick(const Renderer &renderer,
      const Camera &camera,
      const World &world,
      float screenPos_x,
      float screenPos_y) const;

  void *map(OSPFrameBufferChannel channel) const;
  void unmap(void *ptr) const;
  void clear() const;
  float variance() const;
};

static_assert(sizeof(FrameBuffer) == sizeof(OSPFrameBuffer),
    "cpp::FrameBuffer can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline FrameBuffer::FrameBuffer(
    int size_x, int size_y, OSPFrameBufferFormat format, int channels)
{
  ospObject = ospNewFrameBuffer(size_x, size_y, format, channels);
}

inline FrameBuffer::FrameBuffer(const FrameBuffer &copy)
    : ManagedObject<OSPFrameBuffer, OSP_FRAMEBUFFER>(copy.handle())
{
  ospRetain(copy.handle());
}

inline FrameBuffer::FrameBuffer(OSPFrameBuffer existing)
    : ManagedObject<OSPFrameBuffer, OSP_FRAMEBUFFER>(existing)
{}

inline FrameBuffer &FrameBuffer::operator=(const FrameBuffer &copy)
{
  ospObject = copy.ospObject;
  ospRetain(copy.handle());
  return *this;
}

inline void FrameBuffer::resetAccumulation() const
{
  ospResetAccumulation(handle());
}

inline Future FrameBuffer::renderFrame(
    const Renderer &renderer, const Camera &camera, const World &world) const
{
  return ospRenderFrame(
      handle(), renderer.handle(), camera.handle(), world.handle());
}

inline PickResult FrameBuffer::pick(const Renderer &renderer,
    const Camera &camera,
    const World &world,
    float screenPos_x,
    float screenPos_y) const
{
  PickResult res;

  OSPPickResult pick;
  ospPick((OSPPickResult *)&pick,
      handle(),
      renderer.handle(),
      camera.handle(),
      world.handle(),
      screenPos_x,
      screenPos_y);

  if (pick.hasHit) {
    res.hasHit = true;
    res.instance = Instance(pick.instance);
    res.model = GeometricModel(pick.model);
    res.primID = pick.primID;

    std::memcpy(res.worldPosition, pick.worldPosition, 3 * sizeof(float));
  }

  return res;
}

inline void *FrameBuffer::map(OSPFrameBufferChannel channel) const
{
  return const_cast<void *>(ospMapFrameBuffer(handle(), channel));
}

inline void FrameBuffer::unmap(void *ptr) const
{
  ospUnmapFrameBuffer(ptr, handle());
}

inline void FrameBuffer::clear() const
{
  ospResetAccumulation(handle());
}

inline float FrameBuffer::variance() const
{
  return ospGetVariance(handle());
}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::FrameBuffer, OSP_FRAMEBUFFER);

} // namespace ospray
