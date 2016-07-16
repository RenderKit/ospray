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

struct StreamLines {
  uniform Geometry geometry; //!< inherited geometry fields
  float           radius;
  uniform vec3fa *vertex;
  int32           numVertices;
  uniform uint32 *index;
  int32           numSegments;
  uniform vec4f  *color;
};

void StreamLines_bounds(uniform StreamLines *uniform geometry,
                        uniform size_t primID,
                        uniform box3fa &bbox)
{
  uniform uint32 index  = geometry->index[primID];
  const uniform vec3f A = make_vec3f(geometry->vertex[index]);
  const uniform vec3f B = make_vec3f(geometry->vertex[index+1]);
  bbox.lower = make_vec3fa(min(A,B)-make_vec3f(geometry->radius));
  bbox.upper = make_vec3fa(max(A,B)+make_vec3f(geometry->radius));
}

inline bool intersectShiftedSphere(varying Ray &ray,
                                   const vec3f &A, 
                                   const uniform float r,
                                   const uniform int32 primIDtoStore)
{
  const float a = dot(ray.dir,ray.dir);
  const float b = -2.f*dot(ray.dir,A);
  const float c = dot(A,A)-r*r;
  
  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return false;

  const float srad = sqrt(radical);

  const float t_in = (- b - srad) *rcpf(2.f*a);
  const float t_out= (- b + srad) *rcpf(2.f*a);

  if (t_in > ray.t0 && t_in < ray.t) {
    ray.primID = primIDtoStore;
    ray.t = t_in;
    return true;
  } else if (t_out > ray.t0 && t_out < ray.t) {
    ray.primID = primIDtoStore;
    ray.t = t_out;
    return true;
  }
  return false;
}

inline bool intersectShiftedCylinder(varying Ray &ray,
                                     const vec3f &A, const vec3f &B,
                                     const uniform float r,
                                     const uniform int32 primIDtoStore)
{
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
  if (radical < 0.f) return false;
  
  const float srad = sqrt(radical);

  const float t_in = (- b - srad) *rcpf(2.f*a);
  const float t_out= (- b + srad) *rcpf(2.f*a);

  if (t_in >= tAB0 && t_in <= tAB1) {
    ray.primID = primIDtoStore;
    ray.t = t_in;
    return true;
  } else if (t_out >= tAB0 && t_out <= tAB1) {
    ray.primID = primIDtoStore;
    ray.t = t_out;
    return true;
  }
  return false;
}

void StreamLines_intersect(uniform StreamLines *uniform geometry,
                           varying Ray &ray,
                           uniform size_t primID)
{
  const uniform uint32 idx = geometry->index[primID];
  const vec3f A = make_vec3f(geometry->vertex[idx])   - ray.org;
  const vec3f B = make_vec3f(geometry->vertex[idx+1]) - ray.org;

  bool hit = 0;
  
  hit |= intersectShiftedCylinder(ray,A,B,geometry->radius,primID);
  hit |= intersectShiftedSphere(ray,A,geometry->radius,primID | (1<<29));
  hit |= intersectShiftedSphere(ray,B,geometry->radius,primID | (1<<30));

  if (hit) {
    ray.geomID = geometry->geometry.geomID;
    ray.primID = primID; // original primID needed for vertex coloring
    ray.instID = -1;
    const vec3f P = ray.t * ray.dir;
    float s = dot(P-A,B-A) * rcpf(dot(B-A,B-A));
    s = min(max(s,0.f),1.f);
    const vec3f PonAxis = A + s * (B-A);
    ray.u = s;
    ray.Ng = normalize(P-PonAxis);
  }
}

static void StreamLines_postIntersect(uniform Geometry *uniform geometry,
                                       uniform Model *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR)) {
    uniform StreamLines *uniform self = (uniform StreamLines *uniform)geometry;
    uniform vec4f *uniform color = self->color;
    if (color) {
      const varying uint32 index  = self->index[ray.primID];
      dg.color = (1.f-ray.u) * color[index] + ray.u * color[index+1];
    }
  }
}

export void *uniform StreamLineGeometry_create(void           *uniform cppEquivalent)
{
  uniform StreamLines *uniform geom = uniform new uniform StreamLines;
  Geometry_Constructor(&geom->geometry,cppEquivalent,
                       StreamLines_postIntersect,
                       NULL,0,NULL);
  return geom;
}

export void *uniform 
StreamLineGeometry_set(void           *uniform _geom,
                       void           *uniform _model,
                       float           uniform radius,
                       uniform vec3fa *uniform vertex,
                       int32           uniform numVertices,
                       uniform uint32 *uniform index,
                       int32           uniform numSegments,
                       uniform vec4f  *uniform color)
{
  uniform StreamLines *uniform geom = (uniform StreamLines *uniform)_geom;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numSegments);

  geom->geometry.model  = model;
  geom->geometry.geomID = geomID;
  geom->vertex = vertex;
  geom->index = index;
  geom->numSegments = numSegments;
  geom->numVertices = numVertices;
  geom->color = color;
  geom->radius = radius;
  rtcSetUserData(model->embreeSceneHandle,geomID,geom);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&StreamLines_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&StreamLines_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCOccludedFuncVarying)&StreamLines_intersect);
}
