// ospray stuff
#include "renderer.h"
#include "../common/library.h"
// embree stuff
#include "common/sys/library.h"
// stl stuff
#include <map>
// ispc exports
#include "renderer_ispc.h"
// ospray
#include "loadbalancer.h"

namespace ospray {
  typedef Renderer *(*creatorFct)();

  std::map<std::string, creatorFct> rendererRegistry;

  void Renderer::commit()
  {
    spp = getParam1i("spp",1);
    ispc::Renderer_setSPP(this->getIE(),spp);
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
    return (*creator)();
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

  void Renderer::endFrame()
  {
    ispc::Renderer_endFrame(getIE());
  }
  
  void Renderer::renderFrame(FrameBuffer *fb)
  {
    TiledLoadBalancer::instance->renderFrame(this,fb);
  }

};
