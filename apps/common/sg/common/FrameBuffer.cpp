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

    FrameBuffer::FrameBuffer(vec2i size)
    {
      createChild("size", "vec2i", size);
      createChild("displayWall", "string", std::string(""));
      createFB();
    }

    void FrameBuffer::postCommit(RenderContext &)
    {
      std::string displayWall = child("displayWall").valueAs<std::string>();
      this->displayWallStream = displayWall;

      destroyFB();
      createFB();

      if (displayWall != "") {
        ospLoadModule("displayWald");
        OSPPixelOp pixelOp = ospNewPixelOp("display_wald");
        ospSetString(pixelOp,"streamName",displayWall.c_str());
        ospCommit(pixelOp);
        ospSetPixelOp(ospFrameBuffer,pixelOp);
        std::cout << "-------------------------------------------------------"
                  << std::endl;
        std::cout << "this is the display wall frma ebuferr .. size is "
                  << size() << std::endl;
        std::cout << "added display wall pixel op ..." << std::endl;

        std::cout << "created display wall pixelop, and assigned to frame buffer!"
                << std::endl;
      }


      ospCommit(ospFrameBuffer);
    }

    const unsigned char *FrameBuffer::map()
    {
      return (const unsigned char *)ospMapFrameBuffer(ospFrameBuffer,
                                                      OSP_FB_COLOR);
    }

    void FrameBuffer::unmap(const void *mem)
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

    vec2i FrameBuffer::size() const
    {
      return child("size").valueAs<vec2i>();
    }

    /*! \brief returns a std::string with the c++ name of this class */
    std::string FrameBuffer::toString() const
    {
      return "ospray::sg::FrameBuffer";
    }

    OSPFrameBuffer FrameBuffer::handle() const
    {
      return ospFrameBuffer;
    }

    void ospray::sg::FrameBuffer::createFB()
    {
      auto fbsize = size();
      ospFrameBuffer = ospNewFrameBuffer((osp::vec2i&)fbsize,
                                         (displayWallStream=="")
                                         ? OSP_FB_SRGBA
                                         : OSP_FB_NONE,
                                         OSP_FB_COLOR | OSP_FB_ACCUM |
                                         OSP_FB_VARIANCE);
      clearAccum();
      setValue(ospFrameBuffer);
    }

    void ospray::sg::FrameBuffer::destroyFB()
    {
      ospRelease(ospFrameBuffer);
    }

    OSP_REGISTER_SG_NODE(FrameBuffer);

  } // ::ospray::sg
} // ::ospray
