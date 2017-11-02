// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "math/sampling.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct StreamLines {
  uniform Geometry super; //!< inherited geometry fields
  float           radius;
  uniform vec3fa *vertex;
  int32           numVertices;
  uniform uint32 *index;
  uniform vec4f  *color;
};

void StreamLines_bounds(uniform StreamLines *uniform self,
                        uniform size_t primID,
                        uniform box3fa &bbox)
{
  const uniform uint32 index  = self->index[primID];
  const uniform vec3f A = make_vec3f(self->vertex[index]);
  const uniform vec3f B = make_vec3f(self->vertex[index+1]);
  bbox = make_box3fa(min(A,B) - self->radius,
                     max(A,B) + self->radius);
}

/*! do a ray-sphere intersection, with the numerical fix of first
    moving the ray origin to an approximate spher distance, and then
    re-adding this approximate distance to the computed hit
    distance */ 
inline bool intersectSphere_numericalFix(varying Ray &ray,
                                         /*! center/positoin of the sphere  */
                                         const vec3f &center, 
                                         const uniform float r)
{
  const float approximateDistance
    = dot(center-ray.org,ray.dir);// * rsqrtf(dot(ray.dir,ray.dir));

  const vec3f closeOrg = ray.org + approximateDistance * ray.dir;
  const vec3f A = center - closeOrg;
  const float a = dot(ray.dir,ray.dir);
  const float b = -2.f*dot(ray.dir,A);
  const float c = dot(A,A)-r*r;
  
  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return false;

  const float srad = sqrt(radical);

  const float t_in = approximateDistance + (- b - srad) * rcpf(2.f*a);
  const float t_out= approximateDistance + (- b + srad) * rcpf(2.f*a);

  if (t_in > ray.t0 && t_in < ray.t) {
    ray.t = t_in;
    return true;
  } else if (t_out > ray.t0 && t_out < ray.t) {
    ray.t = t_out;
    return true;
  }
  return false;
}

/*! do a ray-cylinder intersection, with the numerical fix of first
    moving the ray origin to an approximate distance, and then
    re-adding this approximate distance to the computed hit
    distance. for the approximate distance we use the distance from
    ray origin to center of the cylinder (projected to the ray) */ 
inline bool intersectCylinder_numericalFix(varying Ray &ray,
                                           const vec3f &_A, const vec3f &_B,
                                           const uniform float r)
{
  const vec3f center = 0.5f*(_A+_B);
  const float approximateDistance
    = dot(center-ray.org,ray.dir); // * rsqrtf(dot(ray.dir,ray.dir));

  const vec3f closeOrg = ray.org + approximateDistance * ray.dir;
  const vec3f A = _A - closeOrg;
  const vec3f B = _B - closeOrg;

  const vec3f V = ray.dir;

  const vec3f AB = B - A;
  
  const vec3f AOxAB = cross(AB,A);
  const vec3f VxAB  = cross(V,AB);
  const float ab2   = dot(AB,AB);
  const float a     = dot(VxAB,VxAB);
  const float b     = 2 * dot(VxAB,AOxAB);
  const float c     = dot(AOxAB,AOxAB) - (sqr(r) * ab2);

  // clip to near and far cap of cylinder
  const float rVAB = rcp(dot(V,AB));
  const float tA = approximateDistance + dot(AB,A) * rVAB;
  const float tB = approximateDistance + dot(AB,B) * rVAB;
  const float tAB0 = max(ray.t0,min(tA,tB));
  const float tAB1 = min(ray.t, max(tA,tB));

  // ------------------------------------------------------------------
  // abc formula: t0,1 = (-b +- sqrt(b^2-4*a*c)) / 2a
  //
  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return false;
  
  const float srad = sqrt(radical);

  const float t_in = approximateDistance + (- b - srad)*rcpf(2.f*a);
  const float t_out= approximateDistance + (- b + srad)*rcpf(2.f*a);

  if (t_in >= tAB0 && t_in <= tAB1) {
    ray.t = t_in;
    return true;
  } else if (t_out >= tAB0 && t_out <= tAB1) {
    ray.t = t_out;
    return true;
  }
  return false;
}

void StreamLines_intersect(uniform StreamLines *uniform self,
                           varying Ray &ray,
                           uniform size_t primID)
{
  const uniform uint32 idx = self->index[primID];
  const vec3f A = make_vec3f(self->vertex[idx]);
  const vec3f B = make_vec3f(self->vertex[idx+1]);

  bool hit = 0;
  hit |= intersectCylinder_numericalFix(ray,A,B,self->radius);
  hit |= intersectSphere_numericalFix(ray,A,self->radius);
  hit |= intersectSphere_numericalFix(ray,B,self->radius);
  if (hit) {
    ray.geomID = self->super.geomID;
    ray.primID = primID;
    const vec3f P = ray.org + ray.t * ray.dir;
    float s = dot(P-A,B-A) * rcpf(dot(B-A,B-A));
    s = min(max(s,0.f),1.f);
    const vec3f PonAxis = A + s * (B-A);
    ray.u = s;
    ray.Ng = normalize(P-PonAxis);
  }
}

static void StreamLines_postIntersect(uniform Geometry *uniform self,
                                       uniform Model *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR)) {
    uniform StreamLines *uniform self = (uniform StreamLines *uniform)self;
    uniform vec4f *uniform color = self->color;
    if (color) {
      const varying uint32 index  = self->index[ray.primID];
      dg.color = (1.f-ray.u) * color[index] + ray.u * color[index+1];
    }
  }
}

// For simplicity streamlines are modelled of segments, each consisting of one
// cylinder and two spheres. Sampling would put (too much) effort towards the
// vertices, where often two spheres and two cylinders overlap. Furthermore,
// sampling such a mixed "primitive" is more complex. Thus, choose a different
// representation for area sampling, i.e. one cylinder per segment and one
// sphere per vertex.
SampleAreaRes StreamLines_sampleArea(
    const Geometry *uniform const _self
    , const int32 areaPrimID
    , const uniform affine3f &xfm
    , const uniform affine3f &rcp_xfm
    , const vec2f& s
    )
{
  const StreamLines *const uniform self = (const StreamLines *uniform)_self;
  const uniform float radius = self->radius;

  vec3f localNormal;
  vec3f localPos;
  if (areaPrimID < self->numVertices) { // sphere
    localNormal = uniformSampleSphere(radius, s);
    const vec3f center = make_vec3f(self->vertex[areaPrimID]);
    localPos = center + localNormal;
  } else { // cylinder
    const uint32 idx = self->index[areaPrimID - self->numVertices];
    const vec3f v0 = make_vec3f(self->vertex[idx]);
    const vec3f v1 = make_vec3f(self->vertex[idx+1]);

    linear3f frame = frame(v1-v0);
    frame.vy = normalize(frame.vy);
    const float phi = two_pi * s.x;
    float sinPhi, cosPhi;
    sincos(phi, &sinPhi, &cosPhi);
    localNormal = (cosPhi*radius)*frame.vx + (sinPhi*radius)*frame.vy;
    localPos = v0 + localNormal + s.y*frame.vz;
  }

  SampleAreaRes res;
  res.pos = xfmPoint(xfm, localPos);
  res.normal = normalize(xfmVector(transposed(rcp_xfm.l), localNormal));

  return res;
}

void StreamLines_getAreas(
    const Geometry *const uniform _self
    , const uniform affine3f &xfm
    , float *const uniform area
    )
{
  StreamLines *uniform self = (StreamLines *uniform)_self;

  // detect (non-uniform) scaling; get length of transformed unit-vectors
  const uniform vec3f scaling3 =
    make_vec3f(length(xfm.l.vx), length(xfm.l.vy), length(xfm.l.vz));
  const uniform float min_scaling = reduce_min(scaling3);
  const uniform float max_scaling = reduce_max(scaling3);
  if ((max_scaling - min_scaling) > 1e-4f*min_scaling)
    print("#osp:StreamLines_getAreas: Non-uniform scaling in instance "
        "transformation detected! Importance sampling for emissive "
        "materials and thus resulting image may be wrong.\n");
  // use average as approximation to closest uniform scaling
  const uniform float scaling = reduce_add(scaling3)/3.f;

  // spheres at vertices
  const uniform float sphereArea = four_pi * sqr(self->radius * scaling);
  const uniform int32 spheres = self->numVertices;
  // initialize with zero per default (i.e. deactivate sampling)
  foreach(i = 0 ... spheres)
    area[i] = 0.f;

  // cylinders for segments; area of a cylinder with unit-length
  const uniform float cyl1Area = two_pi * self->radius * sqr(scaling);
  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  for(uniform int32 i = spheres; i < self->super.primitives; i++) {
    const uniform uint32 idx = self->index[i - spheres];
    const uniform vec3f v0 = make_vec3f(self->vertex[idx]);
    const uniform vec3f v1 = make_vec3f(self->vertex[idx+1]);
    area[i] = cyl1Area * length(v0 - v1);
    // only set area for spheres which are actually part of a segment
    area[idx] = sphereArea;
    area[idx+1] = sphereArea;
  }
}


export void *uniform StreamLines_create(void *uniform cppEquivalent)
{
  StreamLines *uniform geom = uniform new StreamLines;
  Geometry_Constructor(&geom->super,cppEquivalent,
                       StreamLines_postIntersect,
                       NULL,0,NULL);
  return geom;
}

export void *uniform 
StreamLines_set(void *uniform _self,
                       void           *uniform _model,
                       float           uniform radius,
                       uniform vec3fa *uniform vertex,
                       int32           uniform numVertices,
                       uniform uint32 *uniform index,
                       int32           uniform numSegments,
                       uniform vec4f  *uniform color)
{
  StreamLines *uniform self = (StreamLines *uniform)_self;
  Model *uniform model = (Model *uniform)_model;
  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numSegments);

  self->super.model  = model;
  self->super.geomID = geomID;
  self->vertex = vertex;
  self->index = index;
  // XXX different representation for area sampling, see above
  self->super.primitives = numVertices + numSegments;
  self->super.getAreas = StreamLines_getAreas;
  self->super.sampleArea = StreamLines_sampleArea;
  self->numVertices = numVertices;
  self->color = color;
  self->radius = radius;
  rtcSetUserData(model->embreeSceneHandle, geomID, self);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&StreamLines_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&StreamLines_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCOccludedFuncVarying)&StreamLines_intersect);
}
