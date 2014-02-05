// ospray stuff
#include "renderer.h"
// embree stuff
#include "common/sys/library.h"
// stl stuff
#include <map>
// std c stuff
#include <dlfcn.h>
#include "loadbalancer.h"

namespace ospray {
  typedef Renderer *(*creatorFct)();

  std::map<std::string, creatorFct> rendererRegistry;

  Renderer *Renderer::createRenderer(const char *type)
  {
    std::map<std::string, Renderer *(*)()>::iterator it = rendererRegistry.find(type);
    if (it != rendererRegistry.end())
      return it->second ? (it->second)() : NULL;
    
    if (ospray::logLevel >= 2) 
      std::cout << "#ospray: trying to look up renderer type '" 
                << type << "' for the first time" << std::endl;

    std::string creatorName = "ospray_create_renderer__"+std::string(type);
    creatorFct creator = (creatorFct)dlsym(RTLD_DEFAULT,creatorName.c_str());
    rendererRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) 
        std::cout << "#ospray: could not find renderer type '" << type << "'" << std::endl;
      return NULL;
    }
    return (*creator)();
  }

  void TileRenderer::renderFrame(FrameBuffer *fb)
  {
    TiledLoadBalancer::instance->renderFrame(this,fb);
  }
  
};
