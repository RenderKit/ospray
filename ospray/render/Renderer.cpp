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
    model = (Model*)getParamObject("model", getParamObject("world"));
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
                         spp);
    }
  }

  Renderer *Renderer::createRenderer(const char *_type)
  {
    char* type = new char[strlen(_type)+1];
    strcpy(type,_type);
    char *atSign = strstr(type,"@");
    char *libName = NULL;
    if (atSign) {
      *atSign = 0;
      libName = atSign+1;
    }
    if (libName)
      loadLibrary("ospray_module_"+std::string(libName));
    
    std::map<std::string, Renderer *(*)()>::iterator it = rendererRegistry.find(type);
    if (it != rendererRegistry.end()) {
      delete[] type;
      return it->second ? (it->second)() : NULL;
    }
    
    if (ospray::logLevel >= 2) 
      std::cout << "#ospray: trying to look up renderer type '" 
                << type << "' for the first time" << std::endl;

    std::string creatorName = "ospray_create_renderer__"+std::string(type);
    creatorFct creator = (creatorFct)getSymbol(creatorName); //dlsym(RTLD_DEFAULT,creatorName.c_str());
    rendererRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) 
        std::cout << "#ospray: could not find renderer type '" << type << "'" << std::endl;
      delete[] type;
      return NULL;
    }
    delete[] type;
    Renderer *renderer = (*creator)();  renderer->managedObjectType = OSP_RENDERER;
    return(renderer);
  }

  void Renderer::renderTile(Tile &tile)
  {
    ispc::Renderer_renderTile(getIE(),(ispc::Tile&)tile);
  }

  void Renderer::beginFrame(FrameBuffer *fb) 
  {
    this->currentFB = fb;
    ispc::Renderer_beginFrame(getIE(),fb->getIE());
  }

  void Renderer::endFrame(const int32 fbChannelFlags)
  {
    FrameBuffer *fb = this->currentFB;
    if ((fbChannelFlags & OSP_FB_ACCUM))
      fb->accumID++;
    ispc::Renderer_endFrame(getIE(),fb->accumID);
  }
  
  void Renderer::renderFrame(FrameBuffer *fb, const uint32 channelFlags)
  {
    TiledLoadBalancer::instance->renderFrame(this,fb,channelFlags);
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
