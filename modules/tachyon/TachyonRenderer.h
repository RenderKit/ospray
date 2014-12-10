// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

/*! \defgroup ospray_render_tachyon OSPRay's Tachyon-style renderer
  
  \ingroup ospray_supported_renderers

  \brief Implements the data, materials, and lighting models of John
  Stone's "Tachyon" renderer as used by VMD.
 */

// ospray
#include "ospray/render/Renderer.h"

namespace ospray {

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
  struct TachyonRenderer : public Renderer {
    TachyonRenderer();
    virtual void commit();
    virtual std::string toString() const { return "ospray::TachyonRenderer"; }

      Model  *model;
      Camera *camera;
      Data   *textureData;
      Data   *pointLightData;
      void   *pointLightArray;
      uint32  numPointLights;
      Data   *dirLightData;
      void   *dirLightArray;
      uint32  numDirLights;
      bool    doShadows;
  };

} // ::ospray
