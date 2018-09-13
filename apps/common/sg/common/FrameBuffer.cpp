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

#include "FrameBuffer.h"

namespace ospray {
  namespace sg {

    FrameBuffer::FrameBuffer(vec2i size)
    {
      createChild("size", "vec2i", size, NodeFlags::gui_readonly);
      createChild("displayWall", "string", std::string(""));

      std::vector<Any> whiteList;
      for(auto const& el : colorFormats)
        whiteList.push_back(el.first);
      createChild("colorFormat", "string", whiteList[0],
                  NodeFlags::required |
                  NodeFlags::gui_combo,
                  "format of the color buffer");
      child("colorFormat").setWhiteList(whiteList);

      createChild("useAccumBuffer", "bool", true);
      createChild("useVarianceBuffer", "bool", true);

      updateFB();
    }

    void FrameBuffer::postCommit(RenderContext &)
    {
      bool removeToneMapper = false;
      bool toneMapperEnabled = false;
      if (hasChild("toneMapper")) {
        toneMapperEnabled = child("toneMapper").child("enabled").valueAs<bool>();
        if (toneMapperActive && !toneMapperEnabled)
          removeToneMapper = true;
      }
      // Avoid clearing the framebuffer when only tonemapping parameters have been changed
      if (lastModified() >= lastCommitted()
          || child("size").lastModified() >= lastCommitted()
          || child("displayWall").lastModified() >= lastCommitted()
          || child("useAccumBuffer").lastModified() >= lastCommitted()
          || child("useVarianceBuffer").lastModified() >= lastCommitted()
          || child("colorFormat").lastModified() >= lastCommitted()
          || removeToneMapper)
      {
        std::string displayWall = child("displayWall").valueAs<std::string>();
        this->displayWallStream = displayWall;

        updateFB();

        if (displayWall != "") {
          // TODO move into own sg::Node
          ospLoadModule("displayWald");
          OSPPixelOp pixelOp = ospNewPixelOp("display_wald");
          ospSetString(pixelOp, "streamName", displayWall.c_str());
          ospCommit(pixelOp);
          ospSetPixelOp(ospFrameBuffer, pixelOp);
          std::cout << "-------------------------------------------------------"
                    << std::endl;
          std::cout << "this is the display wall framebuffer .. size is "
                    << size() << std::endl;
          std::cout << "added display wall pixel op ..." << std::endl;

          std::cout << "created display wall pixelop, and assigned to frame buffer!"
                  << std::endl;
        }
        ospCommit(ospFrameBuffer);
      }

      if (toneMapperEnabled && !toneMapperActive) {
        ospSetPixelOp(ospFrameBuffer, child("toneMapper").valueAs<OSPPixelOp>());
        toneMapperActive = true;
      }
    }

    const void *FrameBuffer::map()
    {
      return ospMapFrameBuffer(ospFrameBuffer, OSP_FB_COLOR);
    }

    void FrameBuffer::unmap(const void *mem)
    {
      ospUnmapFrameBuffer(mem, ospFrameBuffer);
    }

    void FrameBuffer::clear()
    {
      ospFrameBufferClear(ospFrameBuffer, OSP_FB_ACCUM | OSP_FB_COLOR);
    }

    void FrameBuffer::clearAccum()
    {
      ospFrameBufferClear(ospFrameBuffer, OSP_FB_ACCUM);
    }

    vec2i FrameBuffer::size() const
    {
      return committed_size;
    };

    OSPFrameBufferFormat FrameBuffer::format() const
    {
      return committed_format;
    };

    /*! \brief returns a std::string with the c++ name of this class */
    std::string FrameBuffer::toString() const
    {
      return "ospray::sg::FrameBuffer";
    }

    OSPFrameBuffer FrameBuffer::handle() const
    {
      return ospFrameBuffer;
    };

    void ospray::sg::FrameBuffer::updateFB()
    {
      // workaround insufficient detection of new framebuffer in sg::Frame:
      // create the new FB first and release the old afterwards to ensure a
      // different address / handle
      auto oldFrameBuffer = ospFrameBuffer;

      committed_size = child("size").valueAs<vec2i>();

      committed_format = OSP_FB_NONE;
      auto key = child("colorFormat").valueAs<std::string>();
      for(auto const& el : colorFormats)
        if (el.first == key) {
          committed_format = el.second;
          break;
        }

      auto useAccum    = child("useAccumBuffer").valueAs<bool>();
      auto useVariance = child("useVarianceBuffer").valueAs<bool>();
      ospFrameBuffer = ospNewFrameBuffer((osp::vec2i&)committed_size, committed_format,
                                         OSP_FB_COLOR |
                                         (useAccum ? OSP_FB_ACCUM : 0) |
                                         (useVariance ? OSP_FB_VARIANCE : 0));
      setValue(ospFrameBuffer);
      ospRelease(oldFrameBuffer);
      toneMapperActive = false;
    }


    OSP_REGISTER_SG_NODE(FrameBuffer);

  } // ::ospray::sg
} // ::ospray
