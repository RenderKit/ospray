/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \defgroup ospray_render_raycast A Family of Simple Ray-Cast Renderers
  
  \ingroup ospray_supported_renderers

  \brief Implements a simple renderer that uses a camera given camera
  to shoot (primary) rays, and, based on specified template parameter,
  visualizes a given simple attribute (like normal, texcoords, ...),
  or performs some very simple shading (such as 'eyelight' shading).

  This renderer is mostly intended for debugging, because it allows
  for visualizing certain properties (like geometry location, normal,
  etc) that is otherwise used as inputs for more complex shaders.

  All ray cast renderers support the following two parameters
  <pre>
  Camera "camera"   // the camera used for generating rays
  Model  "world"    // the model used to intersect rays with
  </pre>

  The ray cast renderer is internally implemented as a template over
  the "shade mode", and is publictly exported int the following instantiations:
  <dl>
  <dt>raycast</dt><dd>Performs simple "eyelight" shading</dd>
  <dt>raycast_eyelight</dt><dd>Performs simple "eyelight" shading</dd>
  <dt>raycast_Ng</dt><dd>Visualizes the primary hit's geometry normal (ray.Ng)</dd>
  <dt>raycast_primID</dt><dd>Visualizes (using false-color shading) the primary hit's primitive ID (ray.primID)</dd>
  <dt>raycast_geomID</dt><dd>Visualizes (using false-color shading) the primary hit's geometry ID (ray.geomID)</dd>
  </dl>
 */

// ospray
#include "ospray/render/Renderer.h"

namespace ospray {
  using embree::TaskScheduler;

  struct Camera;
  struct Model;

  // enum { 
  //   RC_EYELIGHT=0,
  //   RC_PRIMID,
  //   RC_GEOMID,
  //   RC_INSTID,
  //   RC_GNORMAL,
  //   RC_TESTSHADOW,
  // } RC_SHADEMODE;

  /*! \brief Implements the family of simple ray cast renderers */
  // template<int SHADE_MODE=RC_EYELIGHT>
  template<void *(*SHADE_MODE)(void*)>
  struct RayCastRenderer : public Renderer {
    RayCastRenderer();
    virtual std::string toString() const { return "ospray::RayCastRenderer"; }

    Model  *model;
    Camera *camera;
    
    // virtual void renderTile(Tile &tile, FrameBuffer *fb);
    virtual void commit();
  };
};
