// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
#include "../common/Library.h"
// stl
#include <map>
// ispc exports
#include "Renderer_ispc.h"
// ospray
#include "LoadBalancer.h"

namespace ospray {
  using std::cout;
  using std::endl;

  typedef Renderer *(*creatorFct)();

  std::map<std::string, creatorFct> rendererRegistry;

  void Renderer::commit()
  {
    epsilon = getParam1f("epsilon", 1e-6f);
    spp = getParam1i("spp", 1);
    backgroundEnabled = getParam1i("backgroundEnabled", 1);
    maxDepthTexture = (Texture2D*)getParamObject("maxDepthTexture", NULL);
    model = (Model*)getParamObject("model", getParamObject("world"));

    if (maxDepthTexture && (maxDepthTexture->type != OSP_FLOAT || !(maxDepthTexture->flags & OSP_TEXTURE_FILTER_NEAREST)))
      static WarnOnce warning("expected maxDepthTexture provided to the renderer to be type OSP_FLOAT and have the OSP_TEXTURE_FILTER_NEAREST flag");

    if (getIE()) {
      ManagedObject* camera = getParamObject("camera");
      if (model) {
        const float diameter = model->bounds.empty() ? 1.0f : length(model->bounds.size());
        epsilon *= diameter;
      }

      ispc::Renderer_set(getIE(),
                         model ?  model->getIE() : NULL,
                         camera ?  camera->getIE() : NULL,
                         epsilon,
                         spp,
                         backgroundEnabled,
                         maxDepthTexture ? maxDepthTexture->getIE() : NULL);
    }
  }

  Renderer *Renderer::createRenderer(const char *_type)
  {
    std::string type = _type;
    size_t atSign = type.find_first_of('@');
    if (atSign != std::string::npos) {
      std::string libName = type.substr(atSign + 1);
      type = type.substr(0, atSign);
      loadLibrary("ospray_module_" + libName);
    }

    std::map<std::string, Renderer *(*)()>::iterator it = rendererRegistry.find(type);
    if (it != rendererRegistry.end()) {
      return it->second ? (it->second)() : NULL;
    }

    if (ospray::logLevel >= 2) {
      std::cout << "#ospray: trying to look up renderer type '"
                << type << "' for the first time" << std::endl;
    }

    std::string creatorName = "ospray_create_renderer__" + type;
    creatorFct creator = (creatorFct)getSymbol(creatorName); //dlsym(RTLD_DEFAULT,creatorName.c_str());
    rendererRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) {
        std::cout << "#ospray: could not find renderer type '" << type << "'" << std::endl;
      }
      return NULL;
    }

    Renderer *renderer = (*creator)();
    renderer->managedObjectType = OSP_RENDERER;
    if (renderer == NULL && ospray::logLevel >= 1) {
      std::cout << "#osp:warning[ospNewRenderer(...)]: could not create renderer of that type." << endl;
      std::cout << "#osp:warning[ospNewRenderer(...)]: Note: Requested renderer type was '" << type << "'" << endl;
    }

    return renderer;
  }

  void Renderer::renderTile(void *perFrameData, Tile &tile, size_t jobID) const
  {
    ispc::Renderer_renderTile(getIE(),perFrameData,(ispc::Tile&)tile, jobID);
  }

  void *Renderer::beginFrame(FrameBuffer *fb)
  {
    this->currentFB = fb;
    return ispc::Renderer_beginFrame(getIE(),fb->getIE());
  }

  void Renderer::endFrame(void *perFrameData, const int32 fbChannelFlags)
  {
    FrameBuffer *fb = this->currentFB;
    if ((fbChannelFlags & OSP_FB_ACCUM))
      fb->accumID++;
    ispc::Renderer_endFrame(getIE(),perFrameData,fb->accumID);
  }

  void Renderer::renderFrame(FrameBuffer *fb, const uint32 channelFlags)
  {
     // double T0 = getSysTime();
    TiledLoadBalancer::instance->renderFrame(this,fb,channelFlags);
     // double T1 = getSysTime();
     // printf("time per frame %lf ms\n",(T1-T0)*1e3f);
  }

  OSPPickResult Renderer::pick(const vec2f &screenPos)
  {
    assert(getIE());
    vec3f pos; bool hit;

    ispc::Renderer_pick(getIE(), (const ispc::vec2f&)screenPos, (ispc::vec3f&)pos, hit);

    OSPPickResult res = { pos, hit };
    return res;
  }

} // ::ospray
