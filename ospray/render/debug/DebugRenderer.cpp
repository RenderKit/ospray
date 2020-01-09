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

// ospray
#include "DebugRenderer.h"
#include "DebugRendererType.h"
// ispc exports
#include "DebugRenderer_ispc.h"

namespace ospray {

  // Helper functions /////////////////////////////////////////////////////////

  static DebugRendererType typeFromString(const std::string &name)
  {
    if (name == "rayDir")
      return DebugRendererType::RAY_DIR;
    else if (name == "eyeLight")
      return DebugRendererType::EYE_LIGHT;
    else if (name == "Ng")
      return DebugRendererType::NG;
    else if (name == "Ns")
      return DebugRendererType::NS;
    else if (name == "backfacing_Ng")
      return DebugRendererType::BACKFACING_NG;
    else if (name == "backfacing_Ns")
      return DebugRendererType::BACKFACING_NS;
    else if (name == "dPds")
      return DebugRendererType::DPDS;
    else if (name == "dPdt")
      return DebugRendererType::DPDT;
    else if (name == "primID")
      return DebugRendererType::PRIM_ID;
    else if (name == "geomID")
      return DebugRendererType::GEOM_ID;
    else if (name == "instID")
      return DebugRendererType::INST_ID;
    else if (name == "volume")
      return DebugRendererType::VOLUME;
    else
      return DebugRendererType::TEST_FRAME;
  }

  // DebugRenderer definitions ////////////////////////////////////////////////

  DebugRenderer::DebugRenderer()
  {
    ispcEquivalent = ispc::DebugRenderer_create(this);
  }

  std::string DebugRenderer::toString() const
  {
    return "ospray::DebugRenderer";
  }

  void DebugRenderer::commit()
  {
    Renderer::commit();
    std::string method = getParam<std::string>("method", "eyeLight");
    ispc::DebugRenderer_set(getIE(), typeFromString(method));
  }

  OSP_REGISTER_RENDERER(DebugRenderer, debug);

}  // namespace ospray
