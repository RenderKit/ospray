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
#include "math/vec.ih"
#include "math/box.ih"
#include "common/Ray.ih"
#include "common/Model.ih"
#include "geometry/Geometry.ih"
#include "math/sampling.ih"

struct StreamLines
{
  uniform Geometry super; //!< inherited geometry fields
  float           radius;
  const uniform vec3fa *vertex;
  int32           numVertices;
  const uniform uint32 *index;
  const uniform vec4f  *color;
};

unmasked void StreamLines_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform StreamLines *uniform self = (uniform StreamLines *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;
  const uniform uint32 index  = self->index[primID];
  const uniform vec3f A = make_vec3f(self->vertex[index]);
  const uniform vec3f B = make_vec3f(self->vertex[index+1]);
  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(min(A,B) - self->radius,
                     max(A,B) + self->radius);
}

// robust ray-sphere intersection
inline void intersectSphere(varying Ray &ray,
                                         const uniform vec3f &center,
                                         float &t_in, float &t_out,
                                         const uniform float r)
{
  const float rd2 = 1.0f / dot(ray.dir, ray.dir); // 1/a
  const vec3f CO = center - ray.org;
  const float projCO = dot(CO, ray.dir)*rd2; // in ray-space
  const vec3f perp = CO - projCO * ray.dir;
  const float l2 = dot(perp, perp);
  const uniform float r2 = sqr(r);
  if (l2 > r2)
    return;

  const float td = sqrt((r2 - l2) * rd2);
  const float sph_t_in = projCO - td;
  const float sph_t_out = projCO + td;

  t_in = min(t_in, sph_t_in);
  t_out = max(t_out, sph_t_out);
}

// robust ray-cylinder intersection
inline void intersectCylinder(varying Ray &ray,
                                           const uniform vec3f &v0, const uniform vec3f &v1,
                                           float &t_in, float &t_out,
                                           const uniform float r)
{
  const vec3f d = ray.dir;
  const uniform vec3f s = v1 - v0; // axis
  const vec3f sxd = cross(s, d);
  const float a = dot(sxd, sxd); // (s x d)^2
  if (a == 0.f)
    return;

  const vec3f f = v0 - ray.org;
  const vec3f sxf = cross(s, f);
  const float ra = 1.0f/a;
  const float ts = dot(sxd, sxf) * ra; // (s x d)(s x f) / (s x d)^2, in ray-space
  const vec3f fp = f - ts * d; // f' = v0 - closest point to axis

  const uniform float s2 = dot(s, s); // s^2
  const vec3f perp = cross(s, fp); // s x f'
  const float c = sqr(r) * s2 - dot(perp, perp); //  r^2 s^2 - (s x f')^2
  if (c < 0.f)
    return;

  float td = sqrt(c * ra);
  const float tin = ts - td;
  const float tout = ts + td;

  // clip to cylinder caps
  const float rsd = rcp(dot(s, d));
  const float tA = dot(s, f) * rsd;
  const float tB = tA + s2 * rsd;
  // rsd (and thus tA and tB) can be NaN if the ray is perfectly perpendicular
  // to the cylinder; filter NaNs by having tin/tout as 2nd argument to min/max
  const float cyl_t_in = max(min(tA, tB), tin);
  const float cyl_t_out = min(max(tA, tB), tout);

  if (cyl_t_in < cyl_t_out) {
    t_in = cyl_t_in;
    t_out = cyl_t_out;
  }
}

unmasked void StreamLines_intersect_kernel(
  const struct RTCIntersectFunctionNArguments *uniform args,
  const uniform bool occlusionTest
)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform StreamLines *uniform self
    = (uniform StreamLines *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  const uniform uint32 idx = self->index[primID];
  const uniform vec3f A = make_vec3f(self->vertex[idx]);
  const uniform vec3f B = make_vec3f(self->vertex[idx+1]);

  float t_in=inf, t_out=-inf;
  intersectCylinder(*ray,A,B,t_in,t_out,self->radius);
  intersectSphere(*ray,A,t_in,t_out,self->radius);
  intersectSphere(*ray,B,t_in,t_out,self->radius);

  bool hit = false;
  if (t_in < t_out) {
    if (t_in > ray->t0 && t_in < ray->t) {
      ray->t = t_in;
      hit = true;
    } else if (t_out > ray->t0 && t_out < ray->t) {
      ray->t = t_out;
      hit = true;
    }
  }

  if (hit) {
    if (occlusionTest) {
      ray->t = neg_inf;
    } else {
      ray->geomID = self->super.geomID;
      ray->primID = primID;
      ray->instID = args->context->instID[0];
      const vec3f P = ray->org + ray->t * ray->dir;
      float s = dot(P-A,B-A) * rcpf(dot(B-A,B-A));
      s = min(max(s,0.f),1.f);
      const vec3f PonAxis = A + s * (B-A);
      ray->u = s;
      ray->Ng = normalize(P-PonAxis);
    }
  }
}

unmasked void StreamLines_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  StreamLines_intersect_kernel(args,false);
}

unmasked void StreamLines_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  StreamLines_intersect_kernel(args,true);
}

static void StreamLines_postIntersect(uniform Geometry *uniform self,
                                       uniform Model *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  StreamLines *uniform self = (StreamLines *uniform)self;

  dg.Ng = dg.Ns = ray.Ng;
  dg.epsilon = self->radius * ulpEpsilon;

  if ((flags & DG_COLOR)) {
    const uniform vec4f *uniform color = self->color;
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
    , const int32 *const uniform primIDs
    , const uniform int32 numPrims
    , const uniform affine3f &xfm
    , float *const uniform areas
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

  // cylinders for segments; area of a cylinder with unit-length
  const uniform float cyl1Area = two_pi * self->radius * sqr(scaling);
  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  for (uniform int32 i = 0; i < numPrims; i++) {
    const uniform int32 primID = primIDs[i];
    if (primID < spheres) { // sphere
      areas[i] = sphereArea;
    } else { // cylinder
      const uniform uint32 idx = self->index[primID - spheres];
      const uniform vec3f v0 = make_vec3f(self->vertex[idx]);
      const uniform vec3f v1 = make_vec3f(self->vertex[idx+1]);
      areas[i] = cyl1Area * length(v0 - v1);
    }
  }
}

export void *uniform StreamLines_create(void *uniform cppEquivalent)
{
  StreamLines *uniform geom = uniform new StreamLines;
  Geometry_Constructor(&geom->super,cppEquivalent,
                       StreamLines_postIntersect,
                       NULL,
                       NULL,
                       0,NULL);
  return geom;
}

export void *uniform
StreamLines_set(       void *uniform _self,
                       void           *uniform _model,
                       float           uniform radius,
                const  uniform vec3fa *uniform vertex,
                       int32           uniform numVertices,
                const  uniform uint32 *uniform index,
                       int32           uniform numSegments,
                const  uniform vec4f  *uniform color)
{
  StreamLines *uniform self = (StreamLines *uniform)_self;
  Model *uniform model = (Model *uniform)_model;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);

  self->super.geomID = geomID;
  self->super.model  = model;
  self->vertex = vertex;
  self->index = index;
  // XXX different representation for area sampling, see above
  self->super.numPrimitives = numVertices + numSegments;
  self->super.getAreas = StreamLines_getAreas;
  self->super.sampleArea = StreamLines_sampleArea;
  self->numVertices = numVertices;
  self->color = color;
  self->radius = radius;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom,numSegments);
  rtcSetGeometryBoundsFunction
    (geom,(uniform RTCBoundsFunction)&StreamLines_bounds, self);
  rtcSetGeometryIntersectFunction
    (geom,(uniform RTCIntersectFunctionN)&StreamLines_intersect);
  rtcSetGeometryOccludedFunction
    (geom,(uniform RTCOccludedFunctionN)&StreamLines_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}

export void *uniform
StreamLines_setCurve(      void           *uniform _self,
                           void           *uniform _model,
                     const uniform vec3fa *uniform vertexCurve,
                           int32           uniform numVertices,
                     const uniform uint32 *uniform indexCurve,
                           int32           uniform numSegments,
                     const uniform uint32 *uniform index,
                     const uniform vec4f  *uniform color)
{
  StreamLines *uniform self = (StreamLines *uniform)_self;
  Model *uniform model = (Model *uniform)_model;

  uniform RTCGeometry geom
    = rtcNewGeometry(ispc_embreeDevice(),
                     RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE);
  rtcSetSharedGeometryBuffer(geom,RTC_BUFFER_TYPE_VERTEX,0,RTC_FORMAT_FLOAT4,
                             vertexCurve,0,sizeof(uniform vec3fa),numVertices);
  rtcSetSharedGeometryBuffer(geom,RTC_BUFFER_TYPE_INDEX,0,RTC_FORMAT_UINT,
                             indexCurve,0,sizeof(uniform int),numSegments);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);

  self->super.geom = geom;

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numSegments;
  self->super.postIntersect = StreamLines_postIntersect;
  // TODO implement area sampling of curves for geometry lights
  self->super.getAreas = NULL;
  self->super.sampleArea = NULL;
  self->numVertices = 0; // not used by curve
  self->vertex = NULL; // not used by curve
  self->index = index;
  self->color = color;
}
