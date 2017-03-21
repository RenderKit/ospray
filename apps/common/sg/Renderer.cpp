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

#include "Renderer.h"

#include "sg/common/FrameBuffer.h"

namespace ospray {
  namespace sg {

    void Renderer::init()
    {
      add(createNode("bounds", "box3f"));
      add(createNode("rendererType", "string", std::string("scivis"),
                     NodeFlags::required | NodeFlags::valid_whitelist | NodeFlags::gui_combo,
                     "scivis: standard whitted style ray tracer. "
                     "pathtracer/pt: photo-realistic path tracer"));
      child("rendererType").setWhiteList({std::string("scivis"),
                                           std::string("sv"),
                                           std::string("raytracer"),
                                           std::string("rt"),
                                           std::string("ao"),
                                           std::string("ao1"),
                                           std::string("ao2"),
                                           std::string("ao4"),
                                           std::string("ao8"),
                                           std::string("ao16"),
                                           std::string("dvr"),
                                           std::string("pathtracer"),
                                           std::string("pt")});
      add(createNode("world", "World"));
      child("world").setDocumentation("model containing scene objects");
      add(createNode("camera", "PerspectiveCamera"));
      add(createNode("frameBuffer", "FrameBuffer"));
      add(createNode("lights"));

      add(createNode("bgColor", "vec3f", vec3f(0.9f, 0.9f, 0.9f),
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));

      //TODO: move these to seperate SciVisRenderer
      add(createNode("shadowsEnabled", "bool", true));
      add(createNode("maxDepth", "int", 5,
                     NodeFlags::required | NodeFlags::valid_min_max,
                     "maximum number of ray bounces"));
      child("maxDepth").setMinMax(0,999);
      add(createNode("aoSamples", "int", 1,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider,
                     "number of ambient occlusion samples.  0 means off"));
      child("aoSamples").setMinMax(0,128);
      add(createNode("spp", "int", 1, NodeFlags::required | NodeFlags::gui_slider,
                     "the number of samples rendered per pixel. The higher "
                     "the number, the smoother the resulting image."));
      child("spp").setMinMax(-8,128);
      add(createNode("aoDistance", "float", 10000.f,
                     NodeFlags::required | NodeFlags::valid_min_max,
                     "maximum distance ao rays will trace to."
                     " Useful if you do not want a large interior of a"
                     " building to be completely black from occlusion."));
      child("aoDistance").setMinMax(float(1e-31),FLT_MAX);
      add(createNode("oneSidedLighting", "bool",true, NodeFlags::required));
      add(createNode("aoTransparency", "bool",true, NodeFlags::required));
    }

    void Renderer::postRender(RenderContext &ctx)
    {
      ospSetObject(ospRenderer, "model",
                   child("world").valueAs<OSPObject>());
      ospCommit(ospRenderer);

      auto fb = (OSPFrameBuffer)child("frameBuffer").valueAs<OSPObject>();
      ospRenderFrame(fb,
                     ospRenderer,
                     OSP_FB_COLOR | OSP_FB_ACCUM);
    }

    void Renderer::preRender(RenderContext& ctx)
    {
      ctx.ospRenderer = ospRenderer;
    }

    void Renderer::preCommit(RenderContext &ctx)
    {
      if (child("frameBuffer")["size"].lastModified() >
          child("camera")["aspect"].lastCommitted()) {
        
        child("camera")["aspect"].setValue(
          child("frameBuffer")["size"].valueAs<vec2i>().x /
          float(child("frameBuffer")["size"].valueAs<vec2i>().y)
        );
      }
      auto rendererType = child("rendererType").valueAs<std::string>();
      if (!ospRenderer || rendererType != createdType) {
        traverse(ctx, "modified");
        ospRenderer = ospNewRenderer(rendererType.c_str());
        assert(ospRenderer);
        createdType = rendererType;
        ospCommit(ospRenderer);
        setValue((OSPObject)ospRenderer);
      }
      ctx.ospRenderer = ospRenderer;
    }

    void Renderer::postCommit(RenderContext &ctx)
    {
      ospSetObject(ospRenderer,"model", child("world").valueAs<OSPObject>());
      ospSetObject(ospRenderer,"camera", child("camera").valueAs<OSPObject>());

      if (lightsData == nullptr ||
          lightsBuildTime < child("lights").childrenLastModified())
      {
        // create and setup light for Ambient Occlusion
        std::vector<OSPLight> lights;
        for(auto &lightNode : child("lights").children())
          lights.push_back((OSPLight)lightNode->valueAs<OSPObject>());

        if (lightsData)
          ospRelease(lightsData);
        lightsData = ospNewData(lights.size(), OSP_LIGHT, &lights[0]);
        ospCommit(lightsData);
        lightsBuildTime = TimeStamp();
      }

      // complete setup of renderer
      ospSetObject(ospRenderer, "model",  child("world").valueAs<OSPObject>());
      ospSetObject(ospRenderer, "lights", lightsData);

      //TODO: some child is kicking off modified every frame...Should figure
      //      out which and ignore it

      if (child("camera").childrenLastModified() > frameMTime
          || child("lights").childrenLastModified() > frameMTime
          || child("world").childrenLastModified() > frameMTime
          || lastModified() > frameMTime
          || child("shadowsEnabled").lastModified() > frameMTime
          || child("aoSamples").lastModified() > frameMTime
          || child("spp").lastModified() > frameMTime)
      {
        ospFrameBufferClear(
          (OSPFrameBuffer)child("frameBuffer").valueAs<OSPObject>(),
          OSP_FB_COLOR | OSP_FB_ACCUM
        );

        frameMTime = TimeStamp();
      }

      ospCommit(ospRenderer);
    }

    OSP_REGISTER_SG_NODE(Renderer);

  } // ::ospray::sg
} // ::ospray
