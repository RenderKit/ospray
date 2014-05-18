#pragma once

// ospray
#include "ospray/render/tilerenderer.h"
// embree
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"

namespace ospray {
  struct Camera;
  struct Model;
  struct Volume;
  
  namespace mhtk {

    /*! \defgroup mhtk_module_xray "XRay" Test Renderer for MHTK Module 
      @ingroup mhtk_module
      @{ */

    /*! \brief Scalar variant of the sample "XRay" renderer to test
        the multi-hit traversal kernel (\ref mhtk_module) */
    struct ScalarXRayRenderer : public TileRenderer {
      virtual std::string toString() const { return "ospray::mhtk::ScalarXRayRenderer"; }
      virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);

      /*! \brief render job for the scalar xray renderer
        Will use scalar (i.e., single-ray based) rendering */
      struct TileJob : public TileRenderer::RenderJob {
        TileJob(Camera *camera, RTCScene scene) 
          : camera(camera), scene(scene) 
        {}
        /*! \brief tile rendering callback */
        virtual void renderTile(Tile &tile);
      
        Camera  *camera;
        RTCScene scene;
      };
    };

    /*! \brief Scalar variant of the sample "XRay" renderer to test
        the multi-hit traversal kernel (\ref mhtk_module) */
    struct ISPCXRayRenderer : public TileRenderer {
      virtual std::string toString() const { return "ospray::mhtk::ISPCXRayRenderer"; }
      virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);
    
      /*! \brief render job for the ispc xray renderer
        Will call ispc-variant of the respective renderile function */
      struct TileJob : public TileRenderer::RenderJob {
        TileJob(Camera *camera, RTCScene scene) 
          : camera(camera), scene(scene) 
        {}
        /*! \brief tile rendering callback */
        virtual void renderTile(Tile &tile);

        Camera  *camera;
        RTCScene scene;
      };
    };

    /*! @} */
  }
}
