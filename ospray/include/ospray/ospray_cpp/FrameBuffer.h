// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
  vec3f worldPosition;
  Instance instance{(OSPInstance) nullptr};
  GeometricModel model{(OSPGeometricModel) nullptr};
  uint32_t primID{0};
};

class FrameBuffer : public ManagedObject<OSPFrameBuffer, OSP_FRAMEBUFFER>
{
 public:
  FrameBuffer() = default; // NOTE(jda) - this does *not* create the underlying
  //            OSP object
  FrameBuffer(const vec2i &size,
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
      const vec2f &screenPos) const;

  void *map(OSPFrameBufferChannel channel) const;
  void unmap(void *ptr) const;
  void clear() const;
};

static_assert(sizeof(FrameBuffer) == sizeof(OSPFrameBuffer),
    "cpp::FrameBuffer can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline FrameBuffer::FrameBuffer(
    const vec2i &size, OSPFrameBufferFormat format, int channels)
{
  ospObject = ospNewFrameBuffer(size.x, size.y, format, channels);
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
    const vec2f &screenPos) const
{
  PickResult res;

  OSPPickResult pick;
  ospPick((OSPPickResult *)&pick,
      handle(),
      renderer.handle(),
      camera.handle(),
      world.handle(),
      screenPos.x,
      screenPos.y);

  if (pick.hasHit) {
    res.hasHit = true;
    res.instance = Instance(pick.instance);
    res.model = GeometricModel(pick.model);
    res.primID = pick.primID;

    std::memcpy(res.worldPosition, pick.worldPosition, sizeof(vec3f));
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

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::FrameBuffer, OSP_FRAMEBUFFER);

} // namespace ospray
