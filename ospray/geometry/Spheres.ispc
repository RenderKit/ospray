// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "common/safe_gather.ih"
#include "geometry/Geometry.ih"
#include "math/sampling.ih"
#include "ospray/OSPDataType.h"

struct Spheres {
  /*! inherit from "Geometry" class: */
  Geometry   super;

  /*! data array that contains the sphere data (possibly but not
    necessarily including the color, which could be in color);
    most offset and stride values point into here.  */
  uint8     *data;
  int32      stride;
  int        offset_center;
  int        offset_radius;
  int        offset_materialID;
  int        offset_colorID;

  float      radius;
  float      epsilon;
  int        materialID;

  uint8      *color;
  int         color_stride;
  int         color_offset;
  int         color_format;

  vec2f     *texcoord;

  bool huge_mesh;
};

inline int32 Spheres_getMaterialID(const uniform Spheres *uniform self,
                                   int32 primID)
{
  int32 materialID = -1;
  if (self->offset_materialID >= 0) {
    const uniform int32 primsPerPage = (1024*1024*64);
    if (any(primID >= primsPerPage )) {
      const int primPageID  = primID / primsPerPage;
      const int localPrimID = primID % primsPerPage;
      foreach_unique (primPage in primPageID) {
        uniform uint8 *uniform pagePtr = self->data
          + (((int64)primPage)
             * primsPerPage
             * self->stride);
        uniform uint8 *varying spherePtr = pagePtr
          + self->stride*localPrimID;
        materialID = *((uniform uint32 *varying)(spherePtr+self->offset_materialID));
      }
    } else {
      uniform uint8 *varying spherePtr = self->data
        + self->stride*primID;
      materialID = *((uniform uint32 *varying)(spherePtr+self->offset_materialID));
    }
  } else {
    materialID = self->materialID;
  }
  return materialID;
}

static void Spheres_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform Spheres *uniform self = (uniform Spheres *uniform)geometry;

  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR) && self->color) {
    uint32 colorID = 0;
    if (self->offset_colorID >= 0) {
      uniform uint8 *varying spherePtr =
        self->data + self->stride*ray.primID;
      colorID = *((uniform uint32 *varying)(spherePtr+self->offset_colorID));
    } else
      colorID = ray.primID;
    if (self->color_format == OSP_FLOAT4) {
      dg.color = gather_byte_stride_vec4f(self->huge_mesh,
                                          self->color + self->color_offset,
                                          self->color_stride,
                                          colorID);
    } else if (self->color_format == OSP_FLOAT3
        || self->color_format == OSP_FLOAT3A)
    {
      vec3f c = gather_byte_stride_vec3f(self->huge_mesh,
                                         self->color + self->color_offset,
                                         self->color_stride,
                                         colorID);
      dg.color = make_vec4f(c.x, c.y, c.z, 1.f);
    } else if (self->color_format == OSP_UCHAR4) {
      const uint32 c = gather_stride_uint32(self->huge_mesh,
                                            self->color + self->color_offset,
                                            self->color_stride,
                                            colorID);
      dg.color.x = (c & 0x000000ff) / 255.0;
      dg.color.y = ((c & 0x0000ff00) >> 8) / 255.0;
      dg.color.z = ((c & 0x00ff0000) >> 16) / 255.0;
      dg.color.w = ((c & 0xff000000) >> 24) / 255.0;
    }
  }

  if (flags & DG_TEXCOORD && self->texcoord)
    dg.st = self->texcoord[ray.primID];

  if (flags & DG_MATERIALID) {
    dg.materialID = Spheres_getMaterialID(self, ray.primID);
  }
}

unmasked void Spheres_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform Spheres *uniform self = (uniform Spheres *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;
  uniform uint8 *uniform spherePtr = self->data
    + self->stride*((uniform int64)primID);
  uniform bool offr = self->offset_radius >= 0;
  uniform float radius =
    offr ? *((uniform float *uniform)(spherePtr+self->offset_radius)) :
    self->radius;
  uniform vec3f center =
    *((uniform vec3f*uniform)(spherePtr+self->offset_center));
  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(center - radius, center + radius);
}

void Spheres_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
                              const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform Spheres *uniform self
    = (uniform Spheres *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  // from here on it's the original intersection code from ospray/embree2
  uniform uint8 *uniform spherePtr =
    self->data + self->stride*((uniform int64)primID);
  uniform float radius = self->radius;
  if (self->offset_radius >= 0) {
    radius = *((uniform float *uniform)(spherePtr+self->offset_radius));
  }
  uniform vec3f center =
    *((uniform vec3f*uniform)(spherePtr+self->offset_center));
  const float approxDist = dot(center - ray->org, ray->dir);
  const vec3f closeOrg = ray->org + approxDist * ray->dir;
  const vec3f A = center - closeOrg;

  const float a = dot(ray->dir,ray->dir);
  const float b = 2.f*dot(ray->dir,A);
  const float c = dot(A,A)-radius*radius;

  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return;

  const float srad = sqrt(radical);

  const float t_in = (b - srad) *rcpf(2.f*a) + approxDist;
  const float t_out= (b + srad) *rcpf(2.f*a) + approxDist;

  bool hit = false;
  if (t_in > ray->t0 && t_in < ray->t) {
    hit = true;
    ray->t = t_in;
  } else if (t_out > (ray->t0 + self->epsilon) && t_out < ray->t) {
    hit = true;
    ray->t = t_out;
  }

  if (hit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->primID = primID;
      ray->geomID = self->super.geomID;
      ray->instID = args->context->instID[0];
      // cannot easily be moved to postIntersect
      // we need hit in object space, in postIntersect it is in world-space
      ray->Ng = ray->org + ray->t*ray->dir - center;
    }
  }
}

unmasked void Spheres_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Spheres_intersect_kernel(args,false);
}

unmasked void Spheres_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Spheres_intersect_kernel(args,true);
}

int32 Spheres_getMaterialID(
                            const Geometry *uniform const _self
                            , const int32 primID
                            )
{
  const Spheres *const uniform self = (const Spheres *uniform)_self;
  return Spheres_getMaterialID(self, primID);
}

SampleAreaRes Spheres_sampleArea(
                                 const Geometry *uniform const _self
                                 , const int32 primID
                                 , const uniform affine3f &xfm
                                 , const uniform affine3f &rcp_xfm
                                 , const vec2f& s
                                 )
{
  const Spheres *const uniform self = (const Spheres *uniform)_self;
  float radius = self->radius;
  vec3f center;

  const uniform int32 primsPerPage = (1024*1024*64);
  if (any(primID >= primsPerPage )) {
    const int primPageID  = primID / primsPerPage;
    const int localPrimID = primID % primsPerPage;
    foreach_unique(primPage in primPageID) {
      uint8 *uniform pagePtr = self->data
        + (((int64)primPage)
           * primsPerPage
           * self->stride);
      uint8 *spherePtr = pagePtr + self->stride*localPrimID;
      center = *((vec3f*)(spherePtr+self->offset_center));
      if (self->offset_radius >= 0)
        radius = *((float *)(spherePtr+self->offset_radius));
    }
  } else {
    uint8 *spherePtr = self->data + self->stride*primID;
    center = *((vec3f*)(spherePtr+self->offset_center));
    if (self->offset_radius >= 0)
      radius = *((float *)(spherePtr+self->offset_radius));
  }

  const vec3f localNormal = uniformSampleSphere(radius, s);
  const vec3f localPos = center + localNormal;

  SampleAreaRes res;
  res.pos = xfmPoint(xfm, localPos);
  res.normal = normalize(xfmVector(transposed(rcp_xfm.l), localNormal));

  return res;
}

void Spheres_getAreas(
                      const Geometry *const uniform _self
                      , const int32 *const uniform primIDs
                      , const uniform int32 numPrims
                      , const uniform affine3f &xfm
                      , float *const uniform areas
                      )
{
  Spheres *uniform self = (Spheres *uniform)_self;

  // detect (non-uniform) scaling; get length of transformed unit-vectors
  const uniform vec3f scaling3 =
    make_vec3f(length(xfm.l.vx), length(xfm.l.vy), length(xfm.l.vz));
  const uniform float min_scaling = reduce_min(scaling3);
  const uniform float max_scaling = reduce_max(scaling3);
  if ((max_scaling - min_scaling) > 1e-4f*min_scaling)
    print("#osp:Spheres_getAreas: Non-uniform scaling in instance "
          "transformation detected! Importance sampling for emissive "
          "materials and thus resulting image may be wrong.\n");
  // use average as approximation to closest uniform scaling
  const uniform float scaling = reduce_add(scaling3)/3.f;

  if (self->offset_radius < 0) {
    const uniform float sphereArea = four_pi * sqr(self->radius * scaling);
    foreach (i = 0 ... numPrims)
      areas[i] = sphereArea;
  } else {
    const uniform int64 stride = self->stride;
    // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
    for (uniform int32 i = 0; i < numPrims; i++) {
      const uniform uint8 *uniform radiusPtr = self->data + self->offset_radius + primIDs[i] * stride;
      const uniform float radius = *((float *uniform)radiusPtr);
      areas[i] = four_pi * sqr(radius * scaling);
    }
  }
}


export void *uniform Spheres_create(void *uniform cppEquivalent)
{
  uniform Spheres *uniform self = uniform new uniform Spheres;
  Geometry_Constructor(&self->super,cppEquivalent,
                       Spheres_postIntersect,
                       NULL,
                       NULL,
                       0,NULL);
  return self;
}

export void SpheresGeometry_set(void  *uniform _self
                                , void *uniform _model
                                , void *uniform data
                                , void *uniform materialList
                                , vec2f *uniform texcoord
                                , void *uniform color
                                , uniform int color_offset
                                , uniform int color_stride
                                , uniform int color_format
                                , uniform int numSpheres
                                , uniform int bytesPerSphere
                                , uniform float radius
                                , uniform int materialID
                                , uniform int offset_center
                                , uniform int offset_radius
                                , uniform int offset_materialID
                                , uniform int offset_colorID
                                , uniform bool huge_mesh
                                )
{
  uniform Spheres *uniform self = (uniform Spheres *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numSpheres;
  self->super.getAreas = Spheres_getAreas;
  self->super.sampleArea = Spheres_sampleArea;
  self->super.materialList = (Material **)materialList;
  self->texcoord = texcoord;
  self->color = (uint8 *uniform)color;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->color_format = color_format;
  self->radius = radius;
  self->data = (uint8 *uniform)data;
  self->materialID = materialID;
  self->stride = bytesPerSphere;

  self->offset_center     = offset_center;
  self->offset_radius     = offset_radius;
  self->offset_materialID = offset_materialID;
  self->offset_colorID    = offset_colorID;

  self->huge_mesh = huge_mesh;

  // Why a log function?
  self->epsilon = log(self->radius);
  if (self->epsilon < 0.f)
    self->epsilon = -1.f/self->epsilon;

  // This can occur if the radius is < 1 in some cases, so make sure
  // epsilon is not too big
  if (self->epsilon > self->radius / 100.0) {
    self->epsilon = self->radius / 100.0;
  }

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom,numSpheres);
  rtcSetGeometryBoundsFunction
    (geom,(uniform RTCBoundsFunction)&Spheres_bounds, self);
  rtcSetGeometryIntersectFunction
    (geom,(uniform RTCIntersectFunctionN)&Spheres_intersect);
  rtcSetGeometryOccludedFunction
    (geom,(uniform RTCOccludedFunctionN)&Spheres_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}
