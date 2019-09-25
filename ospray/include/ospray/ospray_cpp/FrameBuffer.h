// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "Renderer.h"
#include "World.h"

namespace ospray {
  namespace cpp {

    class FrameBuffer : public ManagedObject_T<OSPFrameBuffer>
    {
     public:
      FrameBuffer() =
          default;  // NOTE(jda) - this does *not* create the underlying
      //            OSP object
      FrameBuffer(const vec2i &size,
                  OSPFrameBufferFormat format = OSP_FB_SRGBA,
                  int channels                = OSP_FB_COLOR);
      FrameBuffer(const FrameBuffer &copy);
      FrameBuffer(OSPFrameBuffer existing);

      FrameBuffer &operator=(const FrameBuffer &copy);

      float renderFrame(const Renderer &renderer,
                        const Camera &camera,
                        const World &world) const;

      void *map(OSPFrameBufferChannel channel) const;
      void unmap(void *ptr) const;
      void clear() const;
    };

    // Inlined function definitions ///////////////////////////////////////////

    inline FrameBuffer::FrameBuffer(const vec2i &size,
                                    OSPFrameBufferFormat format,
                                    int channels)
    {
      ospObject = ospNewFrameBuffer(size.x, size.y, format, channels);
    }

    inline FrameBuffer::FrameBuffer(const FrameBuffer &copy)
        : ManagedObject_T<OSPFrameBuffer>(copy.handle())
    {
      ospRetain(copy.handle());
    }

    inline FrameBuffer::FrameBuffer(OSPFrameBuffer existing)
        : ManagedObject_T<OSPFrameBuffer>(existing)
    {
    }

    inline FrameBuffer &FrameBuffer::operator=(const FrameBuffer &copy)
    {
      ospObject = copy.ospObject;
      ospRetain(copy.handle());
      return *this;
    }

    inline float FrameBuffer::renderFrame(const Renderer &renderer,
                                          const Camera &camera,
                                          const World &world) const
    {
      return ospRenderFrame(
          handle(), renderer.handle(), camera.handle(), world.handle());
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

  }  // namespace cpp
}  // namespace ospray
