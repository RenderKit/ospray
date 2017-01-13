// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "FrameBuffer.h"

namespace ospray {
  namespace sg {

    FrameBuffer::FrameBuffer(const vec2i &size) 
      : size(size), 
        ospFrameBuffer(NULL) 
    {
      createFB();
    };
    
    FrameBuffer::~FrameBuffer()
    {
      destroyFB();
    }

    unsigned char *FrameBuffer::map()
    {
      return (unsigned char *)ospMapFrameBuffer(ospFrameBuffer, OSP_FB_COLOR);
    }

    void FrameBuffer::unmap(unsigned char *mem)
    {
      ospUnmapFrameBuffer(mem,ospFrameBuffer);
    }

    void FrameBuffer::clear()
    {
      ospFrameBufferClear(ospFrameBuffer,OSP_FB_ACCUM|OSP_FB_COLOR);
    }

    void FrameBuffer::clearAccum()
    {
      ospFrameBufferClear(ospFrameBuffer,OSP_FB_ACCUM);
    }

    vec2i FrameBuffer::getSize() const
    {
      return size;
    }
    
    /*! \brief returns a std::string with the c++ name of this class */
    std::string FrameBuffer::toString() const
    {
      return "ospray::sg::FrameBuffer";
    }
    
    OSPFrameBuffer FrameBuffer::getOSPHandle() const
    {
      return ospFrameBuffer;
    }

    void ospray::sg::FrameBuffer::createFB()
    {
      ospFrameBuffer =
          ospNewFrameBuffer((const osp::vec2i &)size, OSP_FB_SRGBA,
                            OSP_FB_COLOR | OSP_FB_ACCUM);
    }

    void ospray::sg::FrameBuffer::destroyFB()
    {
      ospFreeFrameBuffer(ospFrameBuffer);
    }

  } // ::ospray::sg
} // ::ospray
