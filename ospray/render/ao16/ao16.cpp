// ospray
#include "ao16.h"
#include "ospray/camera/camera.h"
// embree
#include "common/sys/sync/atomic.h"
// ispc exports
#include "ao16_ispc.h"

namespace ospray {
  
  void AO16Renderer::commit()
  {
    model  = (Model  *)getParamObject("model",NULL); // old naming
    model  = (Model  *)getParamObject("model",model); // new naming
    camera = (Camera *)getParamObject("camera",NULL);
    ispcEquivalent = ispc::AO16Renderer_create(this,model,camera);
  }

  OSP_REGISTER_RENDERER(AO16Renderer,ao16);
};

