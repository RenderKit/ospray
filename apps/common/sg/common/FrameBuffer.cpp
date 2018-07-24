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

      createChild("toneMapping", "bool", true);

      createChild("exposure", "float", 0.0f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(-8.f, 8.f);

      createChild("contrast", "float", 1.6773f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(1.f, 5.f);

      createChild("shoulder", "float", 0.9714f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(0.9f, 1.f);

      createChild("midIn", "float", 0.18f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(0.f, 1.f);

      createChild("midOut", "float", 0.18f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(0.f, 1.f);

      createChild("hdrMax", "float", 11.0785f,
                    NodeFlags::required |
                    NodeFlags::gui_slider).setMinMax(1.f, 64.f);

      createChild("acesColor", "bool", true);

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

      createFB();
    }

    void FrameBuffer::postCommit(RenderContext &)
    {
      bool removeToneMapper = false;
      if (child("toneMapping").lastModified() >= lastCommitted()) {
        bool toneMapping = child("toneMapping").valueAs<bool>();
        if (toneMapping) {
          toneMapper = ospNewPixelOp("tonemapper");
          ospCommit(toneMapper);
          ospSetPixelOp(ospFrameBuffer, toneMapper);
        } else {
          ospRelease(toneMapper);
          toneMapper = nullptr;
          removeToneMapper = true;
        }
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

        destroyFB();
        createFB();

        if (displayWall != "") {
          ospLoadModule("displayWald");
          OSPPixelOp pixelOp = ospNewPixelOp("display_wald");
          ospSetString(pixelOp,"streamName", displayWall.c_str());
          ospCommit(pixelOp);
          ospSetPixelOp(ospFrameBuffer,pixelOp);
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

      if (toneMapper) {
        float exposure = child("exposure").valueAs<float>();
        float linearExposure = exp2(exposure);
        ospSet1f(toneMapper, "exposure", linearExposure);

        ospSet1f(toneMapper, "contrast", child("contrast").valueAs<float>());
        ospSet1f(toneMapper, "shoulder", child("shoulder").valueAs<float>());
        ospSet1f(toneMapper, "midIn", child("midIn").valueAs<float>());
        ospSet1f(toneMapper, "midOut", child("midOut").valueAs<float>());
        ospSet1f(toneMapper, "hdrMax", child("hdrMax").valueAs<float>());
        ospSet1i(toneMapper, "acesColor", child("acesColor").valueAs<bool>());

        ospCommit(toneMapper);
      }
    }

    const void *FrameBuffer::map()
    {
      return ospMapFrameBuffer(ospFrameBuffer, OSP_FB_COLOR);
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

    void ospray::sg::FrameBuffer::createFB()
    {
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
      if (toneMapper)
        ospSetPixelOp(ospFrameBuffer, toneMapper);
      setValue(ospFrameBuffer);
    }

    void ospray::sg::FrameBuffer::destroyFB()
    {
      ospRelease(ospFrameBuffer);
    }

    OSP_REGISTER_SG_NODE(FrameBuffer);

  } // ::ospray::sg
} // ::ospray
