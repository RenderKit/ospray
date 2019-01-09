// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
#include "render/Renderer.h"

namespace ospray {

  /*! \brief Implements the family of simple, primary-ray-only ray cast renderers

    \detailed This simple renderer shoots only a single primary ray
    and does some simple shading, mostly for debugging purposes such
    as visualizing primitive ID, geometry ID, shading normals,
    eyelight shading, etc */
  template<void *(*SHADE_MODE)(void*)>
  struct RaycastRenderer : public Renderer
  {
    RaycastRenderer();
    virtual ~RaycastRenderer() override = default;

    virtual std::string toString() const override;
  };

} // ::ospray
