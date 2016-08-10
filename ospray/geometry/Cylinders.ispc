// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "math/vec.ih"
#include "math/box.ih"
#include "common/Ray.ih"
#include "common/Model.ih"
#include "geometry/Geometry.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Cylinders {
  uniform Geometry geometry; //!< inherited geometry fields

  uniform uint8 *uniform data;
  uniform Material *uniform *materialList;
  uniform vec4f    *uniform color;

  float radius;
  int   materialID;
  int   offset_v0;
  int   offset_v1;
  int   offset_radius;
  int   offset_materialID;
  int   offset_colorID;
  int32 numCylinders;
  int32 bytesPerCylinder;
};

typedef uniform float uniform_float;

unmasked void Cylinders_bounds(uniform Cylinders *uniform geometry,
    uniform size_t primID,
    uniform box3fa &bbox)
{
  uniform uint8 *uniform cylinderPtr = geometry->data + geometry->bytesPerCylinder*primID;
  uniform bool offr = geometry->offset_radius >= 0;
  uniform float radius = offr ? *((uniform float *)(cylinderPtr+geometry->offset_radius)) : geometry->radius;
  uniform vec3f v0 = *((uniform vec3f*)(cylinderPtr+geometry->offset_v0));
  uniform vec3f v1 = *((uniform vec3f*)(cylinderPtr+geometry->offset_v1));
  bbox = make_box3fa(min(v0,v1)-make_vec3f(radius),
                     max(v0,v1)+make_vec3f(radius));
}

void Cylinders_intersect(uniform Cylinders *uniform geometry,
                         varying Ray &ray,
                         uniform size_t primID)
{
  uniform uint8 *uniform cylinderPtr = geometry->data + geometry->bytesPerCylinder*primID;
  uniform float radius = geometry->radius;
  if (geometry->offset_radius >= 0) {
    radius = *((uniform float *)(cylinderPtr+geometry->offset_radius));
  }
  uniform vec3f v0 = *((uniform vec3f*)(cylinderPtr+geometry->offset_v0));
  uniform vec3f v1 = *((uniform vec3f*)(cylinderPtr+geometry->offset_v1));
  const vec3f A = v0 - ray.org;
  const vec3f B = v1 - ray.org;
  const float r = radius;

  const vec3f O = make_vec3f(0.f);
  const vec3f V = ray.dir;

  const vec3f AB = B - A;
  const vec3f AO = O - A;
  
  const vec3f AOxAB  = cross(AO,AB); // cross product
  const vec3f VxAB   = cross(V,AB); // cross product
  const float ab2    = dot(AB,AB); // dot product
  const float a      = dot(VxAB,VxAB); // dot product
  const float b      = 2 * dot(VxAB,AOxAB); // dot product
  const float c      = dot(AOxAB,AOxAB) - (r*r * ab2);

  // clip to near and far cap of cylinder
  const float tA = dot(AB,A) * rcp(dot(V,AB));
  const float tB = dot(AB,B) * rcp(dot(V,AB));
  const float tAB0 = max(ray.t0,min(tA,tB));
  const float tAB1 = min(ray.t, max(tA,tB));

  // ------------------------------------------------------------------
  // abc formula: t0,1 = (-b +- sqrt(b^2-4*a*c)) / 2a
  //
  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return;
  
  const float srad = sqrt(radical);

  const float t_in = (- b - srad) *rcpf(2.f*a);
  const float t_out= (- b + srad) *rcpf(2.f*a);

  bool hit = false;
  if (t_in >= tAB0 && t_in <= tAB1) {
    hit = true;
    ray.t = t_in;
  } else if (t_out >= tAB0 && t_out <= tAB1) {
    hit = true;
    ray.t = t_out;
  }

  if (hit) {
    ray.primID = primID;
    ray.geomID = geometry->geometry.geomID;
    ray.instID = -1;
    // cannot easily be moved to postIntersect
    // we need hit in object space, in postIntersect it is in world-space
    const vec3f P = ray.t*ray.dir - A;
    const vec3f V = cross(P,AB);
    ray.Ng = cross(AB,V);
  }
}


static void Cylinders_postIntersect(uniform Geometry *uniform geometry,
                                       uniform Model *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  uniform Cylinders *uniform self = (uniform Cylinders *uniform)geometry;
  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR) && self->color) {
    if (self->offset_colorID >= 0) {
      uniform uint8 *cylinderPtr = self->data + self->bytesPerCylinder*ray.primID;
      uint32 colorID = *((uniform uint32 *varying)(cylinderPtr+self->offset_colorID));
      dg.color = self->color[colorID];
    } else {
      dg.color = self->color[ray.primID];
    }
  }

  if ((flags & DG_MATERIALID) && (self->offset_materialID >= 0)) {
    uniform uint8 *cylinderPtr = self->data + self->bytesPerCylinder*ray.primID;
    dg.materialID = *((uniform uint32 *varying)(cylinderPtr+self->offset_materialID));
    if (self->materialList) {
      dg.material = self->materialList[dg.materialID];
    }
  }
}

export void *uniform Cylinders_create(void *uniform cppEquivalent)
{
  uniform Cylinders *uniform geom = uniform new uniform Cylinders;
  Geometry_Constructor(&geom->geometry,cppEquivalent,
                       Cylinders_postIntersect,
                       NULL,0,NULL);
  return geom;
}

export void CylindersGeometry_set(void           *uniform _geom,
                                  void           *uniform _model,
                                  void           *uniform data,
                                  void           *uniform materialList,
                                  uniform vec4f  *uniform color,
                                  int             uniform numCylinders,
                                  int             uniform bytesPerCylinder,
                                  float           uniform radius,
                                  int             uniform materialID,
                                  int             uniform offset_v0,
                                  int             uniform offset_v1,
                                  int             uniform offset_radius,
                                  int             uniform offset_materialID,
                                  int             uniform offset_colorID)
{
  uniform Cylinders *uniform geom = (uniform Cylinders *uniform)_geom;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numCylinders);
  
  geom->geometry.model = model;
  geom->geometry.geomID = geomID;
  geom->materialList = (Material **)materialList;
  geom->color = color;
  geom->numCylinders = numCylinders;
  geom->radius = radius;
  geom->data = (uniform uint8 *uniform)data;
  geom->materialID = materialID;
  geom->bytesPerCylinder = bytesPerCylinder;

  geom->offset_v0         = offset_v0;
  geom->offset_v1         = offset_v1;
  geom->offset_radius     = offset_radius;
  geom->offset_materialID = offset_materialID;
  geom->offset_colorID    = offset_colorID;

  rtcSetUserData(model->embreeSceneHandle,geomID,geom);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&Cylinders_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&Cylinders_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&Cylinders_intersect);
}
