#pragma once

/*! \defgroup ospray_render_ao16 Simple 16-sample Ambient Occlusion Renderer
  
  \ingroup ospray_supported_renderers

  \brief Implements a simple renderer that shoots 16 rays (generated
  using a hard-coded set of random numbers) to compute a trivially
  simple and coarse ambient occlusion effect.

  This renderer is intentionally
  simple, and not very sophisticated (e.g., it does not do any
  post-filtering to reduce noise, nor even try and consider
  transparency, material type, etc.

*/

// ospray
#include "ospray/render/Renderer.h"
#include "ospray/texture/Texture.h"

namespace ospray {

  using embree::TaskScheduler;

  struct Camera;
  struct Model;

  struct AO16Material : public Material {
    AO16Material();
    virtual void commit();

    vec3f Kd; /*! diffuse material component, that's all we care for */
    Ref<Texture> map_Kd;
  };
  
  /*! \brief Simple 16-sample Ambient Occlusion Renderer
    
    This renderer uses a set of 16 precomputed AO directions to shoot
    shadow rays; for accumulation these 16 directions are
    (semi-)randomly rotated to give different directions every frame
    (this is far from optimal regarding how well the samples are
    distributed, but seems OK for accumulation).

    To further improve frame rate there's a template parameter
    NUM_SAMPLES_PER_FRAME that allows for selecting subsets of these
    samples (only 1,2,4,8, and 16 are allowed). I.e., whereas AO16
    would usually trace the orginal 16 samples in the first frame, and
    a rotated version of that in the second, with
    NUM_SAMPLES_PER_FRAME set to 8 it would trace the original 8
    samples in the first frame, the next 8 in the second, then
    rotation of the first 8 in the third, etc.
   */
  template<int NUM_SAMPLES_PER_FRAME>
  struct AO16Renderer : public Renderer {
    AO16Renderer();

    virtual std::string toString() const { return "ospray::AO16Renderer"; }
    virtual Material *createMaterial(const char *type) { return new AO16Material; }
    Model  *model;
    Camera *camera;
    // background color
    vec3f bgColor; 

    virtual void commit();
  };

} // ::ospray
