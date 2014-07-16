/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

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
#include "ospray/render/renderer.h"
#include "ospray/texture/texture.h"

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
  
  /*! \brief Simple 16-sample Ambient Occlusion Renderer */
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
};
