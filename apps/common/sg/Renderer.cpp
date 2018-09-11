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

#include "Renderer.h"

#include "common/FrameBuffer.h"
#include "visitor/MarkAllAsModified.h"
#include "visitor/VerifyNodes.h"

#include "ospcommon/memory/malloc.h"

#include "texture/Texture2D.h"

namespace ospray {
  namespace sg {

    static std::vector<std::string> globalWhiteList = {
      std::string("scivis"),
      std::string("pathtracer"),
      std::string("ao"),
      std::string("raycast"),
      std::string("raycast_vertexColor"),
      std::string("raycast_dPds"),
      std::string("raycast_dPdt"),
      std::string("raycast_Ng"),
      std::string("raycast_Ns"),
      std::string("backfacing_Ng"),
      std::string("backfacing_Ns"),
      std::string("primID"),
      std::string("geomID"),
      std::string("instID"),
      std::string("testFrame")
    };

    std::vector<std::string> Renderer::globalRendererTypeWhiteList()
    {
      return globalWhiteList;
    }

    void Renderer::setGlobalRendererTypeWhiteList(std::vector<std::string> list)
    {
      globalWhiteList = list;
    }

    Renderer::Renderer()
    {
      std::string defaultRendererType =
          globalWhiteList.empty() ? "scivis" : globalWhiteList[0];

      createChild("rendererType", "string", defaultRendererType,
                  NodeFlags::required |
                  NodeFlags::gui_combo,
                  "scivis: standard whitted style ray tracer. "
                  "pathtracer/pt: photo-realistic path tracer");

      updateRenderer();

      std::vector<Any> whiteList;
      std::copy(globalWhiteList.begin(),
                globalWhiteList.end(),
                std::back_inserter(whiteList));

      child("rendererType").setWhiteList(whiteList);
      createChild("world",
                  "Model").setDocumentation("model containing scene objects");

      createChild("lights");

      createChild("bgColor", "vec3f", vec3f(0.15f, 0.15f, 0.15f),
                  NodeFlags::required |
                  NodeFlags::gui_color);

      createChild("spp", "int", 1,
                  NodeFlags::required | NodeFlags::gui_slider,
                  "the number of samples rendered per pixel. The higher "
                  "the number, the smoother the resulting image.");
      child("spp").setMinMax(1,128);

      createChild("minContribution", "float", 0.001f,
                  NodeFlags::required |
                  NodeFlags::gui_slider,
                  "sample contributions below this value will be neglected"
                  " to speed-up rendering.");
      child("minContribution").setMinMax(0.f, 0.1f);

      createChild("maxContribution", "float", 5.f,
                  NodeFlags::required |
                  NodeFlags::gui_slider,
                  "sample contributions above this value will be ignored."
                  "  This reduces bright dots appearing in images");
      child("maxContribution").setMinMax(1e-5f, 1e5f);

      createChild("varianceThreshold", "float", 0.f,
                  NodeFlags::required |
                  NodeFlags::gui_slider,
                  "the percent (%) threshold of pixel difference to enable"
                  " tile rendering early termination.");
      child("varianceThreshold").setMinMax(0.f, 25.f);

      //TODO: move these to seperate SciVisRenderer
      createChild("shadowsEnabled", "bool", true);
      createChild("maxDepth", "int", 5,
                  NodeFlags::required | NodeFlags::valid_min_max,
                  "maximum number of ray bounces").setMinMax(0,999);
      createChild("aoSamples", "int", 1,
                  NodeFlags::required |
                  NodeFlags::gui_slider,
                  "AO samples per frame.").setMinMax(0,128);

      createChild("aoDistance", "float", 10000.f,
                  NodeFlags::required,
                  "maximum distance ao rays will trace to."
                  " Useful if you do not want a large interior of a"
                  " building to be completely black from occlusion.");
      child("aoDistance").setMinMax(1e-20f, 1e20f);

      createChild("epsilon", "float", 1e-3f,
                  NodeFlags::required | NodeFlags::valid_min_max,
                  "epsilon step for secondary ray generation.  Adjust"
                  " if you see speckles or a lack of lighting.");
      child("epsilon").setMinMax(1e-20f, 1e20f);
      createChild("autoEpsilon", "bool", true, NodeFlags::required,
        "automatically adjust epsilon step");

      createChild("oneSidedLighting", "bool", true, NodeFlags::required);
      createChild("aoTransparencyEnabled", "bool", true, NodeFlags::required);

      createChild("useGeometryLights", "bool", true, NodeFlags::required);
      createChild("backplate", "Texture2D");
      auto backplate = child("backplate").nodeAs<Texture2D>();
      backplate->size.x = 1;
      backplate->size.y = 1;
      backplate->channels = 3;
      backplate->preferLinear = true;
      backplate->depth = 4;
      const size_t stride = backplate->size.x * backplate->channels * backplate->depth;
      backplate->data = memory::alignedMalloc(sizeof(unsigned char) * backplate->size.y * stride);
      vec3f bgColor = child("bgColor").valueAs<vec3f>();
      memcpy(backplate->data, &bgColor.x, backplate->channels*backplate->depth);
      createChild("useBackplate", "bool", true, NodeFlags::none,
                  "use backplate for path tracer");
    }

    Renderer::~Renderer()
    {
      ospRelease(lightsData);
    }

    void Renderer::renderFrame(std::shared_ptr<FrameBuffer> fb, int flags)
    {
      variance = ospRenderFrame(fb->valueAs<OSPFrameBuffer>(),
                                ospRenderer,
                                flags);
    }

    std::string Renderer::toString() const
    {
      return "ospray::sg::Renderer";
    }

    void Renderer::traverse(RenderContext &ctx, const std::string& operation)
    {
      if (operation == "render") {
        preRender(ctx);
        postRender(ctx);
      }
      else
        Node::traverse(ctx,operation);
    }

    void Renderer::traverse(const std::string& operation)
    {
      Node::traverse(operation);
    }

    void Renderer::preRender(RenderContext& ctx)
    {
      ctx.ospRenderer = ospRenderer;
      ctx.ospRendererType = child("rendererType").valueAs<std::string>();
    }

    void Renderer::preCommit(RenderContext &ctx)
    {
      auto backplate = child("backplate").nodeAs<Texture2D>();
      vec3f bgColor = child("bgColor").valueAs<vec3f>();
      memcpy(backplate->data, &bgColor.x, backplate->channels*backplate->depth);
      backplate->markAsModified();

      ctx.ospRenderer = ospRenderer;
      ctx.ospRendererType = createdType;
      ctx.world = child("world").nodeAs<sg::Model>();
    }

    void Renderer::postCommit(RenderContext &ctx)
    {
      if (lightsData == nullptr ||
          lightsBuildTime < child("lights").childrenLastModified())
      {
        // create and setup light list
        std::vector<OSPLight> lights;
        for(auto &lightNode : child("lights").children()) {
          auto light = lightNode.second->valueAs<OSPLight>();
          if (light)
            lights.push_back(light);
        }

        ospRelease(lightsData);
        lightsData = ospNewData(lights.size(), OSP_LIGHT, &lights[0]);
        ospCommit(lightsData);
        lightsBuildTime.renew();
      }

      // complete setup of renderer
      ospSetObject(ospRenderer, "lights", lightsData);
      ospSetObject(ospRenderer, "backplate", child("backplate").valueAs<OSPObject>());

      auto &world = child("world");

      if (world.childrenLastModified() > lastCommitted()) {
        world.finalize(ctx);
        ospSetObject(ospRenderer, "model", world.valueAs<OSPObject>());
        if (child("autoEpsilon").valueAs<bool>()) {
          const box3f bounds = world["bounds"].valueAs<box3f>();
          const float diam = length(bounds.size());
          float logDiam = ospcommon::log(diam);
          if (logDiam < 0.f) {
            logDiam = -1.f/logDiam;
          }
          const float epsilon = 1e-5f*logDiam;
          ospSet1f(ospRenderer, "epsilon", epsilon);
          ospSet1f(ospRenderer, "aoDistance", diam*0.3);
        }
      }

      if (!child("useBackplate").valueAs<bool>())
        ospSetObject(ospRenderer, "backplate", nullptr);

      ospCommit(ospRenderer);
    }

    OSPPickResult Renderer::pick(const vec2f &pickPos)
    {
      OSPPickResult result;
      ospPick(&result, ospRenderer, (const osp::vec2f&)pickPos);
      return result;
    }

    float Renderer::getLastVariance() const
    {
      return variance;
    }

    void Renderer::updateRenderer()
    {
      auto rendererType = child("rendererType").valueAs<std::string>();
      if (!ospRenderer || rendererType != createdType) {
        auto setRenderer = [&](OSPRenderer handle, const std::string &rType) {
          Node::traverse(MarkAllAsModified{});
          ospRelease(ospRenderer);
          ospRenderer = handle;
          createdType = rType;
          ospCommit(ospRenderer);
          setValue(ospRenderer);
        };

        auto potentialRenderer = ospNewRenderer(rendererType.c_str());
        if (potentialRenderer != nullptr) {
          setRenderer(potentialRenderer, rendererType);
        } else if (ospRenderer == nullptr) {
          //NOTE(jda) - default to scivs!
          setRenderer(ospNewRenderer("scivis"), "scivis");
          child("rendererType").setValue(std::string("scivis"));
        } else {
          //NOTE(jda) - revert rendererType back to name of currently valid
          //            renderer
          child("rendererType").setValue(createdType);
        }
      }
    }

    OSP_REGISTER_SG_NODE(Renderer);

  } // ::ospray::sg
} // ::ospray
