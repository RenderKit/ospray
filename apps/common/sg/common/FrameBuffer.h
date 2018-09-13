// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "Node.h"

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE FrameBuffer : public sg::Node
    {
      /*! constructor allocates an OSP frame buffer object */
      FrameBuffer(vec2i size = vec2i(1024, 768));

      // no destructor since we release the framebuffer object in Node::~Node()

      const void *map();
      void unmap(const void *mem);

      void clear();
      void clearAccum();

      vec2i size() const;
      OSPFrameBufferFormat format() const;

      virtual void postCommit(RenderContext &ctx) override;

      /*! \brief returns a std::string with the c++ name of this class */
      virtual std::string toString() const override;

      OSPFrameBuffer handle() const;

     private:
      // create/update the ospray framebuffer for this class
      void updateFB();

      OSPFrameBuffer ospFrameBuffer {nullptr};
      vec2i committed_size {0};
      OSPFrameBufferFormat committed_format {OSP_FB_NONE};
      std::vector<std::pair<std::string, OSPFrameBufferFormat>> colorFormats {
        {"sRGB",  OSP_FB_SRGBA},
        {"RGBA8", OSP_FB_RGBA8},
        {"float", OSP_FB_RGBA32F},
        {"none",  OSP_FB_NONE}
      };
      std::string displayWallStream;
      bool toneMapperActive {false};
    };

  } // ::ospray::sg
} // ::ospray
