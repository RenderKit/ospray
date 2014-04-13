// obj
#include "objrenderer.h"
#include "objmaterial.h"
// ospray
#include "ospray/common/model.h"
#include "embree2/rtcore.h"

namespace ospray {
  namespace obj {

    void OBJRenderer::RenderTask::renderTile(Tile &tile)
    {
      PING;
      NOTIMPLEMENTED;
    }
    
    TileRenderer::RenderJob *OBJRenderer::createRenderJob(FrameBuffer *fb)
    {
      RenderTask *frame = new RenderTask;
      frame->world = (Model *)getParam("world",NULL);
      Assert2(frame->world,"null world handle (did you forget to assign a "
              "'world' parameter to the ray_cast renderer?)");
      frame->scene = frame->world->eScene;
      Assert2(frame->scene,"invalid model without an embree scene "
              "(did you forget to finalize/'commit' the model?)");

      frame->camera = (Camera *)getParam("camera",NULL);
      Assert2(frame->camera,"null camera handle (did you forget to assign a "
              "'camera' parameter to the ray_cast renderer?)");
      return frame;
    }
    
    /*! \brief create a material of given type */
    Material *OBJRenderer::createMaterial(const std::string &type)
    {
      Assert2(type == "OBJ" || type == "obj","unsupported material type for OBJ Renderer");
      return new OBJMaterial;
    }

    OSP_REGISTER_RENDERER(OBJRenderer,OBJ);
  }
}
