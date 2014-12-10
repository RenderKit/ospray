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
#include "Instance.h"
#include "ospray/common/Model.h"
// ispc exports
#include "Instance_ispc.h"

namespace ospray {

  Instance::Instance()
  {
    this->ispcEquivalent = ispc::InstanceGeometry_create(this);
  }

  void Instance::finalize(Model *model) 
  {
    xfm.l.vx = getParam3f("xfm.l.vx",vec3f(1.f,0.f,0.f));
    xfm.l.vy = getParam3f("xfm.l.vy",vec3f(0.f,1.f,0.f));
    xfm.l.vz = getParam3f("xfm.l.vz",vec3f(0.f,0.f,1.f));
    xfm.p   = getParam3f("xfm.p",vec3f(0.f,0.f,0.f));

    instancedScene = (Model *)getParamObject("model",NULL);
    embreeGeomID = rtcNewInstance(model->embreeSceneHandle,
                                  instancedScene->embreeSceneHandle);

    assert(instancedScene);
    
    const vec3f mesh_center  = xfm.p == vec3f(0.f, 0.f, 0.f)
      ? embree::center(instancedScene->geometry[0]->bounds)
      : xfm.p;
    const vec3f model_center = model->getParam3f("explosion.center", mesh_center);
    const vec3f dir = mesh_center - model_center;
    xfm.p += dir * model->getParam1f("explosion.factor", 0.f);

    rtcSetTransform(model->embreeSceneHandle,embreeGeomID,
                    RTC_MATRIX_COLUMN_MAJOR,
                    (const float *)&xfm);
    rtcEnable(model->embreeSceneHandle,embreeGeomID);
    AffineSpace3f rcp_xfm = rcp(xfm);
    ispc::InstanceGeometry_set(getIE(),
                               (ispc::AffineSpace3f&)xfm,
                               (ispc::AffineSpace3f&)rcp_xfm,
                               instancedScene->getIE());
  }

  OSP_REGISTER_GEOMETRY(Instance,instance);

} // ::ospray
