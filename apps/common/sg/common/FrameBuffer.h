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

#pragma once

// sg components
#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

    struct FrameBuffer : public sg::Node {

      /*! constructor allocates an OSP frame buffer object */
      FrameBuffer(const vec2i &size);

      /*! destructor - relasess the OSP frame buffer object */
      virtual ~FrameBuffer();

      unsigned char *map();
      void unmap(unsigned char *mem);

      void clear();

      void clearAccum();
      
      vec2i getSize() const;

      /*! \brief returns a std::string with the c++ name of this class */
      virtual std::string toString() const;

      OSPFrameBuffer getOSPHandle() const;
      
    // private:
    
      // create the ospray framebuffer for this class
      void createFB();

      // destroy the ospray framebuffer created via createFB()
      void destroyFB();

      OSPFrameBuffer ospFrameBuffer {nullptr};
      const vec2i size;
    };

  } // ::ospray::sg
} // ::ospray
