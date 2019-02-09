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

/*! \defgroup ospray_render_obj "Wavefront OBJ" Material-based Renderer

  \brief Implements the Wavefront OBJ Material Model

  \ingroup ospray_supported_renderers

  This renderer implements a shading model roughly based on the
  Wavefront OBJ material format. In particular, this renderer
  implements the Material model as implemented in \ref
  ospray::OBJMaterial, and implements a recursive ray tracer on top of
  this material model.

  Note that this renderer is NOT fully compatible with the Wavefront
  OBJ specifications - in particular, we do not support the different
  illumination models as specified in the 'illum' field, and always
  perform full ray tracing with the given material parameters.

*/

// ospray
#include "render/Renderer.h"
#include "common/Material.h"

// system
#include <vector>

namespace ospray {

  namespace scivis {

    /*! \brief Renderer for the OBJ Wavefront Material/Lighting format

      See \ref ospray_render_raytracer
    */
    struct SciVisRenderer : public Renderer
    {
      SciVisRenderer();
      virtual ~SciVisRenderer() override = default;
      virtual std::string toString() const override;
      virtual void commit() override;

      std::vector<void*> lightArray; // the 'IE's of the XXXLights
      Data *lightData;
    };

    // Inlined member functions ///////////////////////////////////////////////

    inline std::string SciVisRenderer::toString() const
    {
      return "ospray::SciVisRenderer";
    }

  } // ::ospray::scivis
} // ::ospray

