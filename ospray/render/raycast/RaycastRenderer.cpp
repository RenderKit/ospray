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

#undef NDEBUG

// ospray
#include "RaycastRenderer.h"
#include "ospray/camera/PerspectiveCamera.h"
// embree
#include "common/sys/sync/atomic.h"
// ispc exports
#include "RaycastRenderer_ispc.h"

namespace ospray {

  template<void *(*ISPC_CREATE)(void*)>
  RaycastRenderer<ISPC_CREATE>::RaycastRenderer()
    : model(NULL), camera(NULL) 
  {
    ispcEquivalent = ISPC_CREATE(this);
  }


  template<void *(*ISPC_CREATE)(void*)>
  void RaycastRenderer<ISPC_CREATE>::commit()
  {
    Renderer::commit();

    model = (Model *)getParamObject("world",NULL);
    model = (Model *)getParamObject("model",model);
    camera = (Camera *)getParamObject("camera",NULL);

    ispc::RaycastRenderer_set(getIE(),
                              model?model->getIE():NULL,
                              camera?camera->getIE():NULL);
  }

  typedef RaycastRenderer<ispc::RaycastRenderer_create_eyeLight> RaycastRenderer_eyeLight;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight,raycast);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight,eyeLight);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight,raycast_eyelight);

  typedef RaycastRenderer<ispc::RaycastRenderer_create_Ng> RaycastRenderer_Ng;
  OSP_REGISTER_RENDERER(RaycastRenderer_Ng,raycast_Ng);
  typedef RaycastRenderer<ispc::RaycastRenderer_create_Ns> RaycastRenderer_Ns;
  OSP_REGISTER_RENDERER(RaycastRenderer_Ns,raycast_Ns);

  typedef RaycastRenderer<ispc::RaycastRenderer_create_eyeLight_primID>
  RaycastRenderer_eyeLight_primID;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_primID,eyeLight_primID);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_primID,primID);

  typedef RaycastRenderer<ispc::RaycastRenderer_create_eyeLight_instID>
  RaycastRenderer_eyeLight_instID;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_instID,eyeLight_instID);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_instID,instID);

  typedef RaycastRenderer<ispc::RaycastRenderer_create_eyeLight_geomID>
  RaycastRenderer_eyeLight_geomID;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_geomID,eyeLight_geomID);
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_geomID,geomID);

  typedef RaycastRenderer<ispc::RaycastRenderer_create_eyeLight_vertexColor>
  RaycastRenderer_eyeLight_vertexColor;
  OSP_REGISTER_RENDERER(RaycastRenderer_eyeLight_vertexColor,eyeLight_vertexColor);

  typedef RaycastRenderer<ispc::RaycastRenderer_create_testFrame>
  RaycastRenderer_testFrame;
  OSP_REGISTER_RENDERER(RaycastRenderer_testFrame,testFrame);

} // ::ospray

