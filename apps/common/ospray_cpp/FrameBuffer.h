// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include <ospray_cpp/ManagedObject.h>
#include <ospray_cpp/PixelOp.h>

namespace ospray {
namespace cpp    {

class FrameBuffer : public ManagedObject_T<OSPFrameBuffer>
{
public:

  FrameBuffer() = default;//NOTE(jda) - this does *not* create the underlying
                          //            OSP object
  FrameBuffer(const osp::vec2i &size,
              OSPFrameBufferFormat format = OSP_FB_SRGBA,
              int channels = OSP_FB_COLOR);
  FrameBuffer(const FrameBuffer &copy);
  FrameBuffer(FrameBuffer &&move);
  FrameBuffer(OSPFrameBuffer existing);

  FrameBuffer& operator=(const FrameBuffer &copy);
  FrameBuffer& operator=(      FrameBuffer &&move);

  ~FrameBuffer();

  void setPixelOp(PixelOp &p);
  void setPixelOp(OSPPixelOp p);

  const void *map(OSPFrameBufferChannel channel);
  void unmap(void *ptr);
  void clear(uint32_t channel);

private:

  void free();

  bool m_owner = true;
};

// Inlined function definitions ///////////////////////////////////////////////

inline FrameBuffer::FrameBuffer(const osp::vec2i &size,
                                OSPFrameBufferFormat format,
                                int channels)
{
  m_object = ospNewFrameBuffer(size, format, channels);
}

inline FrameBuffer::FrameBuffer(const FrameBuffer &copy) :
  ManagedObject_T(copy.handle()),
  m_owner(false)
{
}

inline FrameBuffer::FrameBuffer(FrameBuffer &&move) :
  ManagedObject_T(move.handle())
{
  move.m_object = nullptr;
}

inline FrameBuffer::FrameBuffer(OSPFrameBuffer existing) :
  ManagedObject_T(existing)
{
}

inline FrameBuffer& FrameBuffer::operator=(const FrameBuffer &copy)
{
  free();
  m_object = copy.m_object;
  return *this;
}

inline FrameBuffer& FrameBuffer::operator=(FrameBuffer &&move)
{
  free();
  m_object = move.m_object;
  move.m_object = nullptr;
  return *this;
}

inline FrameBuffer::~FrameBuffer()
{
  free();
}

inline void FrameBuffer::setPixelOp(PixelOp &p)
{
  setPixelOp(p.handle());
}

inline void FrameBuffer::setPixelOp(OSPPixelOp p)
{
  ospSetPixelOp(handle(), p);
}

inline const void *FrameBuffer::map(OSPFrameBufferChannel channel)
{
  return ospMapFrameBuffer(handle(), channel);
}

inline void FrameBuffer::unmap(void *ptr)
{
  ospUnmapFrameBuffer(ptr, handle());
}

inline void FrameBuffer::clear(uint32_t channel)
{
  ospFrameBufferClear(handle(), channel);
}

inline void FrameBuffer::free()
{
  if (m_owner && handle()) {
    ospFreeFrameBuffer(handle());
  }
}

}// namespace cpp
}// namespace ospray
