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
#include "Instance.h"
#include "common/Model.h"
// ispc exports
#include "Instance_ispc.h"

namespace ospray {

  Instance::Instance()
  {
    this->ispcEquivalent = ispc::InstanceGeometry_create(this);
  }

  std::string Instance::toString() const
  {
    return "ospray::Instance";
  }

  void Instance::finalize(Model *model)
  {
    xfm.l.vx = getParam3f("xfm.l.vx",vec3f(1.f,0.f,0.f));
    xfm.l.vy = getParam3f("xfm.l.vy",vec3f(0.f,1.f,0.f));
    xfm.l.vz = getParam3f("xfm.l.vz",vec3f(0.f,0.f,1.f));
    xfm.p   = getParam3f("xfm.p",vec3f(0.f,0.f,0.f));

    instancedScene = (Model *)getParamObject("model", nullptr);
    assert(instancedScene);

    if (!instancedScene->embreeSceneHandle) {
      instancedScene->commit();
    }

    RTCGeometry embreeGeom   = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_INSTANCE);
    embreeGeomID = rtcAttachGeometry(model->embreeSceneHandle,embreeGeom);
    rtcSetGeometryInstancedScene(embreeGeom,instancedScene->embreeSceneHandle);

    const box3f b = instancedScene->bounds;
    if (b.empty()) {
      // for now, let's just issue a warning since not all ospray
      // geometries do properly set the bounding box yet. as soon as
      // this gets fixed we will actually switch to reporting an error
      static WarnOnce warning("creating an instance to a model that does not"
                              " have a valid bounding box. epsilons for"
                              " ray offsets may be wrong");
    }
    else {
      const vec3f v000(b.lower.x,b.lower.y,b.lower.z);
      const vec3f v001(b.upper.x,b.lower.y,b.lower.z);
      const vec3f v010(b.lower.x,b.upper.y,b.lower.z);
      const vec3f v011(b.upper.x,b.upper.y,b.lower.z);
      const vec3f v100(b.lower.x,b.lower.y,b.upper.z);
      const vec3f v101(b.upper.x,b.lower.y,b.upper.z);
      const vec3f v110(b.lower.x,b.upper.y,b.upper.z);
      const vec3f v111(b.upper.x,b.upper.y,b.upper.z);

      bounds = empty;
      bounds.extend(xfmPoint(xfm,v000));
      bounds.extend(xfmPoint(xfm,v001));
      bounds.extend(xfmPoint(xfm,v010));
      bounds.extend(xfmPoint(xfm,v011));
      bounds.extend(xfmPoint(xfm,v100));
      bounds.extend(xfmPoint(xfm,v101));
      bounds.extend(xfmPoint(xfm,v110));
      bounds.extend(xfmPoint(xfm,v111));
    }

    rtcSetGeometryTransform(embreeGeom,0,RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR,&xfm);
    rtcCommitGeometry(embreeGeom);
    rtcReleaseGeometry(embreeGeom);

    AffineSpace3f rcp_xfm = rcp(xfm);
    areaPDF.resize(instancedScene->geometry.size());
    ispc::InstanceGeometry_set(getIE(),
                               (ispc::AffineSpace3f&)xfm,
                               (ispc::AffineSpace3f&)rcp_xfm,
                               instancedScene->getIE(),
                               &areaPDF[0]);
    for (auto volume : instancedScene->volume) {
      ospSet3f((OSPObject)volume.ptr, "xfm.l.vx", xfm.l.vx.x, xfm.l.vx.y, xfm.l.vx.z);
      ospSet3f((OSPObject)volume.ptr, "xfm.l.vy", xfm.l.vy.x, xfm.l.vy.y, xfm.l.vy.z);
      ospSet3f((OSPObject)volume.ptr, "xfm.l.vz", xfm.l.vz.x, xfm.l.vz.y, xfm.l.vz.z);
      ospSet3f((OSPObject)volume.ptr, "xfm.p", xfm.p.x, xfm.p.y, xfm.p.z);
    }
  }

  OSP_REGISTER_GEOMETRY(Instance,instance);

} // ::ospray
