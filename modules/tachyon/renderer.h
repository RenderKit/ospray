#pragma once

/*! \defgroup ospray_render_tachyon OSPRay's Tachyon-style renderer
  
  \ingroup ospray_supported_renderers

  \brief Implements the data, materials, and lighting models of John
  Stone's "Tachyon" renderer as used by VMD.
 */

// ospray
#include "ospray/render/tilerenderer.h"

namespace ospray {
  using embree::TaskScheduler;

  struct Camera;
  struct Model;

  enum { 
    RC_EYELIGHT=0,
    RC_PRIMID,
    RC_GEOMID,
    RC_INSTID,
    RC_GNORMAL,
    RC_TESTSHADOW,
  } RC_SHADEMODE;

  /*! \brief Implements the family of simple ray cast renderers */
  struct TachyonRenderer : public TileRenderer {
    virtual std::string toString() const { return "ospray::TachyonRenderer"; }

    struct RenderTask : public TileRenderer::RenderJob {
      Model  *world;
      Camera *camera;
      Data   *textureData;
      Data   *pointLightData;
      void   *pointLightArray;
      uint32  numPointLights;
      Data   *dirLightData;
      void   *dirLightArray;
      uint32  numDirLights;

      virtual void renderTile(Tile &tile);
      virtual ~RenderTask() {}
    };
    virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);
  private:
  };
};
