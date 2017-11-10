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
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct CylinderTexCoord {
  vec2f v0;
  vec2f v1;
};
struct Cylinders {
  Geometry super; //!< inherited geometry fields

  uint8 *data;
  Material **materialList;

  float radius;
  int   materialID;
  int   offset_v0;
  int   offset_v1;
  int   offset_radius;
  int   offset_materialID;
  int   offset_colorID;
  int32 bytesPerCylinder;
  float epsilon;

  uint8 *color;
  int    color_stride;
  bool   has_alpha; // 4th color component is valid
 
  CylinderTexCoord *texcoord;
};

unmasked void Cylinders_bounds(Cylinders *uniform self,
    uniform size_t primID,
    uniform box3fa &bbox)
{
  uniform uint8 *uniform cylinderPtr = self->data + self->bytesPerCylinder*primID;
  uniform bool offr = self->offset_radius >= 0;
  uniform float radius = offr ? *((float *uniform)(cylinderPtr + self->offset_radius)) : self->radius;
  uniform vec3f v0 = *((vec3f *uniform)(cylinderPtr + self->offset_v0));
  uniform vec3f v1 = *((vec3f *uniform)(cylinderPtr + self->offset_v1));
  // TODO less conservative bounds
  bbox = make_box3fa(min(v0,v1) - radius,
                     max(v0,v1) + radius);
}

void Cylinders_intersect(Cylinders *uniform self,
                         varying Ray &ray,
                         uniform size_t primID)
{
  uniform uint8 *uniform cylinderPtr = self->data + self->bytesPerCylinder*primID;
  uniform float radius = self->radius;
  if (self->offset_radius >= 0) {
    radius = *((float *uniform)(cylinderPtr + self->offset_radius));
  }
  uniform vec3f v0 = *((vec3f *uniform)(cylinderPtr + self->offset_v0));
  uniform vec3f v1 = *((vec3f *uniform)(cylinderPtr + self->offset_v1));

  const vec3f center = 0.5f*(v0 + v1);
  const float approxDist = dot(center - ray.org, ray.dir);
  const vec3f closeOrg = ray.org + approxDist * ray.dir;

  const vec3f A = v0 - closeOrg;
  const vec3f B = v1 - closeOrg;

  const vec3f V = ray.dir;
  const vec3f AB = B - A;
  
  const vec3f AOxAB  = cross(AB,A);
  const vec3f VxAB   = cross(V,AB);
  const float ab2    = dot(AB,AB);
  const float a      = dot(VxAB,VxAB);
  const float b      = 2 * dot(VxAB,AOxAB);
  const float c      = dot(AOxAB,AOxAB) - (sqr(radius) * ab2);

  // clip to near and far cap of cylinder
  const float rVAB = rcp(dot(V,AB));
  const float tA = dot(AB,A) * rVAB + approxDist;
  const float tB = dot(AB,B) * rVAB + approxDist;
  const float tAB0 = max(ray.t0,min(tA,tB));
  const float tAB1 = min(ray.t, max(tA,tB));

  // ------------------------------------------------------------------
  // abc formula: t0,1 = (-b +- sqrt(b^2-4*a*c)) / 2a
  //
  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return;
  
  const float srad = sqrt(radical);

  const float t_in = (- b - srad) *rcpf(2.f*a) + approxDist;
  const float t_out= (- b + srad) *rcpf(2.f*a) + approxDist;

  bool hit = false;

  if (t_in >= (tAB0) && t_in <= (tAB1)) {
    hit = true;
    ray.t = t_in;
  } else if (t_out >= (tAB0+self->epsilon) && t_out <= (tAB1)) {
    hit = true;
    ray.t = t_out;
  }

  if (hit) {
    ray.primID = primID;
    ray.geomID = self->super.geomID;
    // cannot easily be moved to postIntersect
    // we need hit in object-space, in postIntersect it is in world-space
    const vec3f P = ray.org + ray.t*ray.dir - v0;
    const vec3f V = cross(P,AB);
    ray.Ng = cross(AB,V);
    ray.u = (ray.t-tA)*rcp(tB-tA);
  }
}


static void Cylinders_postIntersect(Geometry *uniform _self,
                                    Model *uniform model,
                                    varying DifferentialGeometry &dg,
                                    const varying Ray &ray,
                                    uniform int64 flags)
{
  uniform Cylinders *uniform self = (uniform Cylinders *uniform)_self;
  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR) && self->color) {
    uint32 colorID = 0;
    if (self->offset_colorID >= 0) {
      uniform uint8 *cylinderPtr = self->data + self->bytesPerCylinder*ray.primID;
      colorID = *((uniform uint32 *varying)(cylinderPtr+self->offset_colorID));
    } else
      colorID = ray.primID;
    dg.color = *((vec4f *)(self->color + colorID*self->color_stride));
    if (!self->has_alpha)
      dg.color.w = 1.f;
  }

  if (flags & DG_TEXCOORD && self->texcoord) {
    CylinderTexCoord tc = self->texcoord[ray.primID];
    dg.st = lerp(ray.u, tc.v0, tc.v1);
  } else
    dg.st = make_vec2f(0.0f);
  
  if ((flags & DG_MATERIALID) && (self->offset_materialID >= 0)) {
    uniform uint8 *cylinderPtr = self->data + self->bytesPerCylinder*ray.primID;
    dg.materialID = *((uniform uint32 *varying)(cylinderPtr+self->offset_materialID));
    if (self->materialList) {
      dg.material = self->materialList[dg.materialID];
    }
  }
}

SampleAreaRes Cylinders_sampleArea(
    const Geometry *uniform const _self
    , const int32 primID
    , const uniform affine3f &xfm
    , const uniform affine3f &rcp_xfm
    , const vec2f& s
    )
{
  const Cylinders *const uniform self = (const Cylinders *uniform)_self;
  uint8 *cylinderPtr = self->data + self->bytesPerCylinder*primID;
  float radius = self->radius;
  if (self->offset_radius >= 0)
    radius = *((float *)(cylinderPtr + self->offset_radius));
  vec3f v0 = *((vec3f *)(cylinderPtr + self->offset_v0));
  vec3f v1 = *((vec3f *)(cylinderPtr + self->offset_v1));

  linear3f frame = frame(v1-v0);
  frame.vy = normalize(frame.vy);
  const float phi = two_pi * s.x;
  float sinPhi, cosPhi;
  sincos(phi, &sinPhi, &cosPhi);
  const vec3f localNormal = (cosPhi*radius)*frame.vx + (sinPhi*radius)*frame.vy;
  const vec3f localPos = v0 + localNormal + s.y*frame.vz;

  SampleAreaRes res;
  res.pos = xfmPoint(xfm, localPos);
  res.normal = normalize(xfmVector(transposed(rcp_xfm.l), localNormal));

  return res;
}

void Cylinders_getAreas(
    const Geometry *const uniform _self
    , const uniform affine3f &xfm
    , float *const uniform area
    )
{
  Cylinders *uniform self = (Cylinders *uniform)_self;

  // detect (non-uniform) scaling; get length of transformed unit-vectors
  const uniform vec3f scaling3 =
    make_vec3f(length(xfm.l.vx), length(xfm.l.vy), length(xfm.l.vz));
  const uniform float min_scaling = reduce_min(scaling3);
  const uniform float max_scaling = reduce_max(scaling3);
  if ((max_scaling - min_scaling) > 1e-4f*min_scaling)
    print("#osp:Cylinders_getAreas: Non-uniform scaling in instance "
        "transformation detected! Importance sampling for emissive "
        "materials and thus resulting image may be wrong.\n");
  // use average as approximation to closest uniform scaling
  const uniform float scaling = reduce_add(scaling3)/3.f;

  uniform float radius = self->radius;
  uint8 *uniform cylinderPtr = self->data;
  const uniform int64 stride = self->bytesPerCylinder;
  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  for(uniform int32 i = 0; i < self->super.primitives; i++, cylinderPtr += stride) {
    if (self->offset_radius >= 0)
      radius = *((float *uniform)(cylinderPtr + self->offset_radius));
    const uniform float circ = two_pi * radius * scaling;
    uniform vec3f v0 = *((vec3f *uniform)(cylinderPtr + self->offset_v0));
    uniform vec3f v1 = *((vec3f *uniform)(cylinderPtr + self->offset_v1));
    area[i] = circ * length(v0 - v1) * scaling;
  }
}


export void *uniform Cylinders_create(void *uniform cppEquivalent)
{
  uniform Cylinders *uniform geom = uniform new uniform Cylinders;
  Geometry_Constructor(&geom->super,cppEquivalent,
                       Cylinders_postIntersect,
                       NULL,0,NULL);
  return geom;
}

export void CylindersGeometry_set(void *uniform _self
    , void *uniform _model
    , void *uniform data
    , void *uniform materialList
    , void *uniform texcoord
    , void *uniform color
    , uniform int color_stride
    , uniform bool has_alpha
    , uniform int numCylinders
    , uniform int bytesPerCylinder
    , uniform float radius
    , uniform int materialID
    , uniform int offset_v0
    , uniform int offset_v1
    , uniform int offset_radius
    , uniform int offset_materialID
    , uniform int offset_colorID
    )
{
  Cylinders *uniform self = (Cylinders *uniform)_self;
  Model *uniform model = (Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numCylinders);
  
  self->super.model = model;
  self->super.geomID = geomID;
  self->super.primitives = numCylinders;
  self->super.getAreas = Cylinders_getAreas;
  self->super.sampleArea = Cylinders_sampleArea;
  self->materialList = (Material **)materialList;
  self->texcoord = (CylinderTexCoord *uniform)texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->has_alpha = has_alpha;
  self->radius = radius;
  self->data = (uint8 *uniform)data;
  self->materialID = materialID;
  self->bytesPerCylinder = bytesPerCylinder;

  self->offset_v0         = offset_v0;
  self->offset_v1         = offset_v1;
  self->offset_radius     = offset_radius;
  self->offset_materialID = offset_materialID;
  self->offset_colorID    = offset_colorID;

  self->epsilon = log(self->radius);
  if (self->epsilon < 0.f)
    self->epsilon = -1.f/self->epsilon;

  rtcSetUserData(model->embreeSceneHandle, geomID, self);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&Cylinders_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&Cylinders_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&Cylinders_intersect);
}
