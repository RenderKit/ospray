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

// sg components
#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

    struct FrameBuffer : public sg::Node {
      vec2i size;
      OSPFrameBuffer ospFrameBuffer;

      FrameBuffer(const vec2i &size) 
        : size(size), 
          ospFrameBuffer(NULL) 
      { createFB(); };

      unsigned char *map() { return (unsigned char *)ospMapFrameBuffer(ospFrameBuffer, OSP_FB_COLOR); }
      void unmap(unsigned char *mem) { ospUnmapFrameBuffer(mem,ospFrameBuffer); }

      void clear() 
      {
        ospFrameBufferClear(ospFrameBuffer,OSP_FB_ACCUM|OSP_FB_COLOR);
      }

      void clearAccum() 
      {
        ospFrameBufferClear(ospFrameBuffer,OSP_FB_ACCUM);
      }
      
      vec2i getSize() const { return size; }

      virtual ~FrameBuffer() { destroyFB(); }

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::FrameBuffer"; }
      
    private:
      // create the ospray framebuffer for this class
      void createFB();

      // destroy the ospray framebuffer created via createFB()
      void destroyFB();
    };

    // -------------------------------------------------------
    // IMPLEMENTTATION
    // -------------------------------------------------------

    inline void FrameBuffer::createFB() 
    {
      ospFrameBuffer = ospNewFrameBuffer((const osp::vec2i&)size, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
    }
    
    inline void FrameBuffer::destroyFB() 
    {
      ospFreeFrameBuffer(ospFrameBuffer); 
    }

    
  } // ::ospray::sg
} // ::ospray
