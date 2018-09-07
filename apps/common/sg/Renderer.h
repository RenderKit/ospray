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

#include "SceneGraph.h"

namespace ospray {
  namespace sg {

    class FrameBuffer;

    /*! renderer renders the scene into the framebuffer on render call.
     * child render calls are made during commit to cache rendering.
     * verify and commit must be called before rendered
    */
    struct OSPSG_INTERFACE Renderer : public Renderable
    {
      Renderer();
      ~Renderer() override;
      virtual std::string toString() const override;

      void renderFrame(std::shared_ptr<FrameBuffer> fb,
                       int flags = OSP_FB_COLOR | OSP_FB_ACCUM);

      virtual void traverse(RenderContext &ctx,
                            const std::string& operation) override;

      // NOTE(jda) - why does this overload NOT show up from the base Node???
      void traverse(const std::string &operation);

      void preRender(RenderContext &ctx) override;
      void preCommit(RenderContext &ctx) override;
      void postCommit(RenderContext &ctx) override;
      OSPPickResult pick(const vec2f &pickPos);
      float getLastVariance() const;

      // Global whitelist values for rendererType //

      static std::vector<std::string> globalRendererTypeWhiteList();
      static void setGlobalRendererTypeWhiteList(std::vector<std::string> list);

    private:

      friend struct Frame;

      // Helper functions //

      void updateRenderer();

      // Data members //

      OSPRenderer ospRenderer {nullptr};
      OSPData lightsData {nullptr};
      TimeStamp lightsBuildTime;
      float variance {inf};
      std::string createdType = "none";
    };

  } // ::ospray::sg
} // ::ospray
