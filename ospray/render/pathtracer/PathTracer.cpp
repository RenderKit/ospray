#undef NDEBUG

// ospray
#include "PathTracer.h"
#include "PathTracer_ispc.h"
// std
#include <map>

namespace ospray {
  PathTracer::PathTracer()
    : Renderer()
  {
    const int32 maxDepth = 20;
    const float minContribution = .01f;
    const float epsilon = 1e-3f;
    void *backplate = NULL;
    ispcEquivalent = ispc::PathTracer_create(this,maxDepth,minContribution,epsilon,
                                             backplate);

  }

  /*! \brief create a material of given type */
  Material *PathTracer::createMaterial(const char *type) 
  { 
    std::string ptType = std::string("PathTracer_")+type;
    Material *material = Material::createMaterial(ptType.c_str());
    if (!material) {
      std::map<std::string,int> numOccurrances;
      const std::string T = type;
      if (numOccurrances[T] == 0) 
        std::cout << "#osp:pt: do not know material type '" << type << "'" << 
          " (replacing with OBJMaterial)" << std::endl;
      numOccurrances[T] ++;
      material = Material::createMaterial("PathTracer_OBJMaterial");
      // throw std::runtime_error("invalid path tracer material "+std::string(type)); 
    }
    material->refInc();
    return material;
  }

  void PathTracer::commit() 
  {
    Renderer::commit();
    model = (Model*)getParamObject("world",NULL);
    model = (Model*)getParamObject("model",model.ptr);

    if (model)
      ispc::PathTracer_setModel(getIE(),model->getIE());

    camera = (Camera*)getParamObject("camera",NULL);
    if (camera) 
      ispc::PathTracer_setCamera(getIE(),camera->getIE());
  }

  OSPPickData PathTracer::unproject(const vec2f &screenPos)
  {
    assert(getIE());
    ispc::OSPPickData data_t = ispc::PathTracer_unproject(getIE(), (const ispc::vec2f&)screenPos);
    OSPPickData ret = { data_t.hit, data_t.world_x, data_t.world_y, data_t.world_z };
    return ret;
  }

  OSP_REGISTER_RENDERER(PathTracer,pathtracer);
  OSP_REGISTER_RENDERER(PathTracer,pt);

  extern "C" void ospray_init_module_pathtracer() 
  {
    printf("Loaded plugin 'pathtracer' ...\n");
  }
};

