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
#include "GeometryInstance.h"
// ispc exports
#include "GeometryInstance_ispc.h"

namespace ospray {

  GeometryInstance::GeometryInstance(Geometry *geometry)
  {
    if (geometry == nullptr)
      throw std::runtime_error("NULL Geometry given to GeometryInstance!");

    instancedGeometry = geometry;

    this->ispcEquivalent =
        ispc::GeometryInstance_create(this, geometry->getIE());
  }

  GeometryInstance::~GeometryInstance()
  {
    if (embreeSceneHandle)
      rtcReleaseScene(embreeSceneHandle);
  }

  std::string GeometryInstance::toString() const
  {
    return "ospray::GeometryInstance";
  }

  void GeometryInstance::commit()
  {
    if (embreeSceneHandle)
      rtcReleaseScene(embreeSceneHandle);

    embreeSceneHandle = rtcNewScene(ispc_embreeDevice());

    xfm.l.vx = getParam3f("xfm.l.vx", vec3f(1.f, 0.f, 0.f));
    xfm.l.vy = getParam3f("xfm.l.vy", vec3f(0.f, 1.f, 0.f));
    xfm.l.vz = getParam3f("xfm.l.vz", vec3f(0.f, 0.f, 1.f));
    xfm.p    = getParam3f("xfm.p", vec3f(0.f, 0.f, 0.f));

    bool useEmbreeDynamicSceneFlag = getParam<int>("dynamicScene", 0);
    bool useEmbreeCompactSceneFlag = getParam<int>("compactMode", 0);
    bool useEmbreeRobustSceneFlag  = getParam<int>("robustMode", 0);

    int sceneFlags = 0;
    sceneFlags |= (useEmbreeDynamicSceneFlag ? RTC_SCENE_FLAG_DYNAMIC : 0);
    sceneFlags |= (useEmbreeCompactSceneFlag ? RTC_SCENE_FLAG_COMPACT : 0);
    sceneFlags |= (useEmbreeRobustSceneFlag ? RTC_SCENE_FLAG_ROBUST : 0);

    rtcSetSceneFlags(embreeSceneHandle, static_cast<RTCSceneFlags>(sceneFlags));
  }

  void GeometryInstance::finalize(World *world)
  {
#if 0
    RTCGeometry embreeGeom =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_INSTANCE);

    embreeGeomID = rtcAttachGeometry(world->embreeSceneHandle, embreeGeom);

    rtcSetGeometryInstancedScene(embreeGeom,
                                 instancedGeometry->embreeSceneHandle);

    const box3f b = instancedGeom->bounds;
    const vec3f v000(b.lower.x, b.lower.y, b.lower.z);
    const vec3f v001(b.upper.x, b.lower.y, b.lower.z);
    const vec3f v010(b.lower.x, b.upper.y, b.lower.z);
    const vec3f v011(b.upper.x, b.upper.y, b.lower.z);
    const vec3f v100(b.lower.x, b.lower.y, b.upper.z);
    const vec3f v101(b.upper.x, b.lower.y, b.upper.z);
    const vec3f v110(b.lower.x, b.upper.y, b.upper.z);
    const vec3f v111(b.upper.x, b.upper.y, b.upper.z);

    bounds = empty;
    bounds.extend(xfmPoint(xfm, v000));
    bounds.extend(xfmPoint(xfm, v001));
    bounds.extend(xfmPoint(xfm, v010));
    bounds.extend(xfmPoint(xfm, v011));
    bounds.extend(xfmPoint(xfm, v100));
    bounds.extend(xfmPoint(xfm, v101));
    bounds.extend(xfmPoint(xfm, v110));
    bounds.extend(xfmPoint(xfm, v111));

    rtcSetGeometryTransform(
        embreeGeom, 0, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &xfm);
    rtcCommitGeometry(embreeGeom);
    rtcReleaseGeometry(embreeGeom);

    AffineSpace3f rcp_xfm = rcp(xfm);
    ispc::GeometryInstance_set(
        getIE(), (ispc::AffineSpace3f &)xfm, (ispc::AffineSpace3f &)rcp_xfm);
#endif
  }

}  // namespace ospray
