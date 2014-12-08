/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// ospray stuff
#include "Renderer.h"
#include "../common/Library.h"
// stl stuff
#include <map>
// ispc exports
#include "Renderer_ispc.h"
// ospray
#include "loadbalancer.h"

namespace ospray {
  typedef Renderer *(*creatorFct)();

  std::map<std::string, creatorFct> rendererRegistry;

  void Renderer::commit()
  {
    spp = getParam1i("spp",1);
    nearClip = getParam1f("near_clip",1e-6f);
    if(this->getIE()) {
      ispc::Renderer_setSPP(this->getIE(),spp);
      ispc::Renderer_setNearClip(this->getIE(), nearClip);
    }
  }

  Renderer *Renderer::createRenderer(const char *type)
  {
    std::map<std::string, Renderer *(*)()>::iterator it = rendererRegistry.find(type);
    if (it != rendererRegistry.end())
      return it->second ? (it->second)() : NULL;
    
    if (ospray::logLevel >= 2) 
      std::cout << "#ospray: trying to look up renderer type '" 
                << type << "' for the first time" << std::endl;

    std::string creatorName = "ospray_create_renderer__"+std::string(type);
    creatorFct creator = (creatorFct)getSymbol(creatorName); //dlsym(RTLD_DEFAULT,creatorName.c_str());
    rendererRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) 
        std::cout << "#ospray: could not find renderer type '" << type << "'" << std::endl;
      return NULL;
    }
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

  OSPPickData Renderer::unproject(const vec2f &screenPos)
  {
    assert(getIE());
    bool hit; float x, y, z;

    ispc::Renderer_unproject(getIE(),(const ispc::vec2f&)screenPos, hit, x, y, z);
    OSPPickData ret = { hit, x, y, z };
    
    return ret;
  }

};
