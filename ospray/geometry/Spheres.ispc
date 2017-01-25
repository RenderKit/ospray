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

struct Spheres {
  /*! inherit from "Geometry" class: */
  Geometry   super; 

  /*! data array that contains the sphere data (possibly but not
      necessarily including the color, which could be in color_data);
      all of offset and stride values point into here.  */
  uint8     *data;
  
  /*! list of multiple  materials, in case of per-sphere materials */
  Material **materialList;

  float      radius;
  int        materialID;
  int        offset_center;
  int        offset_radius;
  int        offset_materialID;
  int        offset_colorID; 

  uint8     *color_data;
  int        color_stride;
  int        color_offset;
  
  int32      bytesPerSphere;
};

static void Spheres_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform Spheres *uniform self = (uniform Spheres *uniform)geometry;

  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR) && self->color_data) {
    uint32 colorID = 0;
    if (self->offset_colorID >= 0) {
      uniform uint8 *varying spherePtr =
        self->data + self->bytesPerSphere*ray.primID;
      colorID = *((uniform uint32 *varying)(spherePtr+self->offset_colorID));
    } else {
      colorID = ray.primID;
    }
    uint32 colorAddr = self->color_offset+colorID*self->color_stride;
    dg.color = *((vec4f *)(self->color_data+colorAddr));
  }
  
  if (flags & DG_MATERIALID) {
    if (self->offset_materialID >= 0) {
      const uniform int32 primsPerPage = (1024*1024*64);
      if (any(ray.primID >= primsPerPage )) {
        const int primPageID  = ray.primID / primsPerPage;
        const int localPrimID = ray.primID % primsPerPage;
        foreach_unique(primPage in primPageID) {
          uniform uint8 *uniform pagePtr   = self->data
            + (((int64)primPage)
               * primsPerPage
               * self->bytesPerSphere);
          uniform uint8 *varying spherePtr = pagePtr
            + self->bytesPerSphere*localPrimID;
          dg.materialID =
            *((uniform uint32 *varying)(spherePtr+self->offset_materialID));
          if (self->materialList) {
            dg.material = self->materialList[dg.materialID];
          }
        }
      } else {
        uniform uint8 *varying spherePtr = self->data
          + self->bytesPerSphere*ray.primID;
        dg.materialID =
          *((uniform uint32 *varying)(spherePtr+self->offset_materialID));
        if (self->materialList) {
          dg.material = self->materialList[dg.materialID];
        }
      }
    } else {
      dg.materialID = self->materialID;
      if (self->materialList) {
        dg.material = self->materialList[dg.materialID];
      }
    }
  }
}

unmasked void Spheres_bounds(uniform Spheres *uniform self,
                             uniform size_t primID,
                             uniform box3fa &bbox)
{
  uniform uint8 *uniform spherePtr = self->data
    + self->bytesPerSphere*((uniform int64)primID);
  uniform bool offr = self->offset_radius >= 0;
  uniform float radius =
    offr ? *((uniform float *uniform)(spherePtr+self->offset_radius)) :
    self->radius;
  uniform vec3f center =
    *((uniform vec3f*uniform)(spherePtr+self->offset_center));
  bbox = make_box3fa(center - radius, center + radius);
}

void Spheres_intersect(uniform Spheres *uniform self,
                       varying Ray &ray,
                       uniform size_t primID)
{
  uniform uint8 *uniform spherePtr =
    self->data + self->bytesPerSphere*((uniform int64)primID);
  uniform float radius = self->radius;
  if (self->offset_radius >= 0) {
    radius = *((uniform float *uniform)(spherePtr+self->offset_radius));
  }
  uniform vec3f center =
    *((uniform vec3f*uniform)(spherePtr+self->offset_center));
  const vec3f A = center - ray.org;

  const float a = dot(ray.dir,ray.dir);
  const float b = 2.f*dot(ray.dir,A);
  const float c = dot(A,A)-radius*radius;
  
  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return;

  const float srad = sqrt(radical);

  const float t_in = (b - srad) *rcpf(2.f*a);
  const float t_out= (b + srad) *rcpf(2.f*a);

  bool hit = false;
  if (t_in > ray.t0 && t_in < ray.t) {
    hit = true;
    ray.t = t_in;
  } else if (t_out > ray.t0 && t_out < ray.t) {
    hit = true;
    ray.t = t_out;
  }
  if (hit) {
    ray.primID = primID;
    ray.geomID = self->super.geomID;
    // cannot easily be moved to postIntersect
    // we need hit in object space, in postIntersect it is in world-space
    ray.Ng = ray.org + ray.t*ray.dir - center;
  }
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
            * self->bytesPerSphere);
      uint8 *spherePtr = pagePtr + self->bytesPerSphere*localPrimID;
      center = *((vec3f*)(spherePtr+self->offset_center));
      if (self->offset_radius >= 0)
        radius = *((float *)(spherePtr+self->offset_radius));
    }
  } else {
    uint8 *spherePtr = self->data + self->bytesPerSphere*primID;
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
    , const uniform affine3f &xfm
    , float *const uniform area
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

  const uniform int32 spheres = self->super.primitives;
  if (self->offset_radius < 0) {
    const uniform float sphereArea = four_pi * sqr(self->radius * scaling);
    foreach(i = 0 ... spheres)
      area[i] = sphereArea;
  } else {
    const uniform uint8 *uniform radiusPtr = self->data + self->offset_radius;
    const uniform int64 stride = self->bytesPerSphere;
    // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
    for(uniform int32 i = 0; i < spheres; i++, radiusPtr += stride) {
      const uniform float radius = *((float *uniform)radiusPtr);
      area[i] = four_pi * sqr(radius * scaling);
    }
  }
}


export void *uniform Spheres_create(void *uniform cppEquivalent)
{
  uniform Spheres *uniform self = uniform new uniform Spheres;
  Geometry_Constructor(&self->super,cppEquivalent,
                       Spheres_postIntersect,
                       NULL,0,NULL);
  return self;
}

export void SpheresGeometry_set(void  *uniform _self,
                                void  *uniform _model,
                                void  *uniform data,
                                void  *uniform materialList,
                                uint8 *uniform color_data,
                                int    uniform color_offset,
                                int    uniform color_stride,
                                int    uniform numSpheres,
                                int    uniform bytesPerSphere,
                                float  uniform radius,
                                int    uniform materialID,
                                int    uniform offset_center,
                                int    uniform offset_radius,
                                int    uniform offset_materialID,
                                int    uniform offset_colorID)
{
  uniform Spheres *uniform self = (uniform Spheres *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numSpheres);
  
  self->super.model = model;
  self->super.geomID = geomID;
  self->super.primitives = numSpheres;
  self->super.getAreas = Spheres_getAreas;
  self->super.sampleArea = Spheres_sampleArea;
  self->materialList = (Material **)materialList;
  self->color_data = color_data;
  self->color_stride = color_stride;
  self->color_offset = color_offset;
  self->radius = radius;
  self->data = (uniform uint8 *uniform)data;
  self->materialID = materialID;
  self->bytesPerSphere = bytesPerSphere;

  self->offset_center     = offset_center;
  self->offset_radius     = offset_radius;
  self->offset_materialID = offset_materialID;
  self->offset_colorID    = offset_colorID;

  rtcSetUserData(model->embreeSceneHandle,geomID,self);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&Spheres_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&Spheres_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&Spheres_intersect);
}
