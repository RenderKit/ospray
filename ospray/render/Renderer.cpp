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

// ospray
#include "Renderer.h"
#include "common/Util.h"
// ispc exports
#include "Renderer_ispc.h"
// ospray
#include "LoadBalancer.h"

namespace ospray {

  std::string Renderer::toString() const 
  {
    return "ospray::Renderer";
  }

  void Renderer::commit()
  {
    autoEpsilon = getParam1i("autoEpsilon", true);
    epsilon = getParam1f("epsilon", 1e-6f);
    spp = std::max(1, getParam1i("spp", 1));
    const int32 maxDepth = std::max(0, getParam1i("maxDepth", 20));
    const float minContribution = getParam1f("minContribution", 0.001f);
    errorThreshold = getParam1f("varianceThreshold", 0.f);
    maxDepthTexture = (Texture2D*)getParamObject("maxDepthTexture", nullptr);
    model = (Model*)getParamObject("model", getParamObject("world"));

    if (maxDepthTexture) {
      if (maxDepthTexture->type != OSP_TEXTURE_R32F
          || !(maxDepthTexture->flags & OSP_TEXTURE_FILTER_NEAREST)) {
        static WarnOnce warning("maxDepthTexture provided to the renderer "
                                "needs to be of type OSP_TEXTURE_R32F and have "
                                "the OSP_TEXTURE_FILTER_NEAREST flag");
      }
    }

    vec3f bgColor3 = getParam3f("bgColor", vec3f(getParam1f("bgColor", 0.f)));
    bgColor = getParam4f("bgColor", vec4f(bgColor3, 0.f));

    if (getIE()) {
      ManagedObject* camera = getParamObject("camera");
      if (model) {
        const float diameter = model->bounds.empty() ?
                               1.0f : length(model->bounds.size());
        epsilon *= diameter;
      }

      ispc::Renderer_set(getIE()
          , model ? model->getIE() : nullptr
          , camera ? camera->getIE() : nullptr
          , autoEpsilon
          , epsilon
          , spp
          , maxDepth
          , minContribution
          , (ispc::vec4f&)bgColor
          , maxDepthTexture ? maxDepthTexture->getIE() : nullptr
          );
    }
  }

  Renderer *Renderer::createInstance(const char *type)
  {
    return createInstanceHelper<Renderer, OSP_RENDERER>(type);
  }

  void Renderer::renderTile(void *perFrameData, Tile &tile, size_t jobID) const
  {
    ispc::Renderer_renderTile(getIE(),perFrameData,(ispc::Tile&)tile, jobID);
  }

  void *Renderer::beginFrame(FrameBuffer *fb)
  {
    this->currentFB = fb;
    fb->beginFrame();
    return ispc::Renderer_beginFrame(getIE(),fb->getIE());
  }

  void Renderer::endFrame(void *perFrameData, const int32 /*fbChannelFlags*/)
  {
    ispc::Renderer_endFrame(getIE(),perFrameData);
  }

  float Renderer::renderFrame(FrameBuffer *fb, const uint32 channelFlags)
  {
    return TiledLoadBalancer::instance->renderFrame(this,fb,channelFlags);
  }

  OSPPickResult Renderer::pick(const vec2f &screenPos)
  {
    assert(getIE());

    OSPPickResult res;
    ispc::Renderer_pick(getIE(),
                        (const ispc::vec2f&)screenPos,
                        (ispc::vec3f&)res.position,
                        res.hit);

    return res;
  }

} // ::ospray
