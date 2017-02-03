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
    epsilon = getParam1f("epsilon", 1e-6f);
    spp = getParam1i("spp", 1);
    errorThreshold = getParam1f("varianceThreshold", 0.f);
    backgroundEnabled = getParam1i("backgroundEnabled", 1);
    maxDepthTexture = (Texture2D*)getParamObject("maxDepthTexture", nullptr);
    model = (Model*)getParamObject("model", getParamObject("world"));

    if (maxDepthTexture) {
      if (maxDepthTexture->type != OSP_TEXTURE_R32F
          || !(maxDepthTexture->flags & OSP_TEXTURE_FILTER_NEAREST)) {
        static WarnOnce warning("expected maxDepthTexture provided to the "
                                "renderer to be type OSP_TEXTURE_R32F and have "
                                "the OSP_TEXTURE_FILTER_NEAREST flag");
      }
    }

    vec3f bgColor;
    bgColor = getParam3f("bgColor", vec3f(1.f));

    if (getIE()) {
      ManagedObject* camera = getParamObject("camera");
      if (model) {
        const float diameter = model->bounds.empty() ?
                               1.0f : length(model->bounds.size());
        epsilon *= diameter;
      }

      ispc::Renderer_set(getIE(),
                         model ? model->getIE() : nullptr,
                         camera ? camera->getIE() : nullptr,
                         epsilon,
                         spp,
                         backgroundEnabled,
                         (ispc::vec3f&)bgColor,
                         maxDepthTexture ? maxDepthTexture->getIE() : nullptr);
    }
  }

  Renderer *Renderer::createRenderer(const char *type)
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
     // double T0 = getSysTime();
    return TiledLoadBalancer::instance->renderFrame(this,fb,channelFlags);
     // double T1 = getSysTime();
     // printf("time per frame %lf ms\n",(T1-T0)*1e3f);
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
