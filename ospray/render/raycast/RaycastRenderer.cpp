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

#undef NDEBUG

// ospray
#include "RaycastRenderer.h"
#include "camera/PerspectiveCamera.h"
// ispc exports
#include "RaycastRenderer_ispc.h"

namespace ospray {

  /*! \brief constructor */
  template<void *(*ISPC_CREATE)(void*)>
  RaycastRenderer<ISPC_CREATE>::RaycastRenderer()
  {
    ispcEquivalent = ISPC_CREATE(this);
  }

  //! \brief common function to help printf-debugging
  /*! \detailed Every derived class should override this! */
  template<void *(*ISPC_CREATE)(void*)>
  std::string RaycastRenderer<ISPC_CREATE>::toString() const
  {
    return "ospray::RaycastRenderer";
  }

  /*! \brief Instantion of Ray Cast Renderer that does a simple "eyelight" shading */
  /*! \detailed "eyelight" shading means to take the dot product
      between ray and shading normal (cosTheta), and shade with
      M*(C_a+C_d*cosTheta), where M is some sort of material value
      (usually, the diffuse material component, but can also be a
      value computed from primID etc), C_a is a ambient base lighting
      term (usually around 0.2 or 0.3), and C_d is a lighting term
      that scales the cosTheta angle (usually around .8) to give the
      effect of a point light at the viewer's position */
  using RaycastRenderer_eyeLight = RaycastRenderer<ispc::RaycastRenderer_create_eyeLight>;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight,raycast);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight,eyeLight);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight,raycast_eyelight);

  /*! \brief Instantion of Ray Cast Renderer that shows dg.Ng (geometry normal of hit) */
  using RaycastRenderer_Ng = RaycastRenderer<ispc::RaycastRenderer_create_Ng>;
  OSP_REGISTER_RENDERER(RaycastRenderer_Ng,raycast_Ng);

  /*! \brief Instantion of Ray Cast Renderer that shows dg.Ns (shading normal of hit) */
  using RaycastRenderer_Ns = RaycastRenderer<ispc::RaycastRenderer_create_Ns>;
  OSP_REGISTER_RENDERER(RaycastRenderer_Ns,raycast_Ns);

  /*! \brief Instantion of Ray Cast Renderer that shows dg.dPds (tangent wrt. texcoord s) */
  using RaycastRenderer_dPds = RaycastRenderer<ispc::RaycastRenderer_create_dPds>;
  OSP_REGISTER_RENDERER(RaycastRenderer_dPds,raycast_dPds);

  /*! \brief Instantion of Ray Cast Renderer that shows dg.dPdt (tangent wrt. texcoord t) */
  using RaycastRenderer_dPdt = RaycastRenderer<ispc::RaycastRenderer_create_dPdt>;
  OSP_REGISTER_RENDERER(RaycastRenderer_dPdt,raycast_dPdt);

  /*! \brief Instantion of Ray Cast Renderer that shows dg.primID (primitive ID of hit) */
  using RaycastRenderer_eyeLight_primID = RaycastRenderer<ispc::RaycastRenderer_create_eyeLight_primID>;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_primID,eyeLight_primID);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_primID,primID);

  /*! \brief Instantion of Ray Cast Renderer that shows dg.instID (instance ID of hit) */
  using RaycastRenderer_eyeLight_instID = RaycastRenderer<ispc::RaycastRenderer_create_eyeLight_instID>;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_instID,eyeLight_instID);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_instID,instID);

  /*! \brief Instantion of Ray Cast Renderer that shows eye-light
      shading with a material color based on dg.geomID */
  using RaycastRenderer_eyeLight_geomID = RaycastRenderer<ispc::RaycastRenderer_create_eyeLight_geomID>;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_geomID,eyeLight_geomID);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_geomID,geomID);

  /*! \brief Instantion of Ray Cast Renderer that shows eye-light
      shading with a material color based on interpolated dg.color
      (usually, vertex colors) */
  using RaycastRenderer_eyeLight_vertexColor = RaycastRenderer<ispc::RaycastRenderer_create_eyeLight_vertexColor>;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_vertexColor,raycast_vertexColor);

  /*! \brief Instantion of Ray Cast Renderer that highlights backfacing
   * surfaces (wrt. geometry normal / triangle orientation) in pink */
  using RaycastRenderer_backfacing_Ng = RaycastRenderer<ispc::RaycastRenderer_create_backfacing_Ng>;
  OSP_REGISTER_RENDERER(RaycastRenderer_backfacing_Ng,backfacing_Ng);

  /*! \brief Instantion of Ray Cast Renderer that highlights surfaces that are
   * "inside" (determined by the shading normal) in pink */
  using RaycastRenderer_backfacing_Ns = RaycastRenderer<ispc::RaycastRenderer_create_backfacing_Ns>;
  OSP_REGISTER_RENDERER(RaycastRenderer_backfacing_Ns,backfacing_Ns);

  /*! \brief Instantion of Ray Cast Renderer that renders a simple
      test frame, without even calling postIntersct */
  using RaycastRenderer_testFrame = RaycastRenderer<ispc::RaycastRenderer_create_testFrame>;
  OSP_REGISTER_RENDERER(RaycastRenderer_testFrame,testFrame);

  /*! \brief Instantion of Ray Cast Renderer that renders a simple
      test frame, without even calling postIntersct */
  using RaycastRenderer_rayDir = RaycastRenderer<ispc::RaycastRenderer_create_rayDir>;
  OSP_REGISTER_RENDERER(RaycastRenderer_rayDir,rayDir);

} // ::ospray

