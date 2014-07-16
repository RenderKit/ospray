/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// ospray
#include "ospray/math/vec.ih"
#include "ospray/math/bbox.ih"
#include "ospray/common/ray.ih"
#include "ospray/geometry/geometry.ih"
#include "ospray/common/model.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Disks {
  uniform Geometry geometry; //!< inherited geometry fields

  uniform uint8 *uniform data;

  float           radius;
  int             materialID;
  int             offset_center;
  int             offset_normal;
  int             offset_radius;
  int             offset_materialID;
  int32           numDisks;
  int32           bytesPerDisk;
};

typedef uniform float uniform_float;

static void Disks_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform Disks *uniform THIS = (uniform Disks *uniform)geometry;
  dg.geometry = geometry;
  dg.material = geometry->material;
  vec3f Ng = ray.Ng;
  vec3f Ns = Ng;

  if (flags & DG_NORMALIZE) {
    Ng = normalize(Ng);
    Ns = normalize(Ns);
  }
  if (flags & DG_FACEFORWARD) {
    if (dot(ray.dir,Ng) >= 0.f) Ng = neg(Ng);
    if (dot(ray.dir,Ns) >= 0.f) Ns = neg(Ns);
  }
  if ((flags & DG_MATERIALID) && (THIS->offset_materialID >= 0)) {
    uniform uint8 *diskPtr = 
      THIS->data + THIS->bytesPerDisk*ray.primID;
    dg.materialID = *((uniform uint32 *varying)(diskPtr+THIS->offset_materialID));
  }
  dg.Ng = Ng;
  dg.Ns = Ns;
}

void Disks_bounds(uniform Disks *uniform geometry,
                    uniform size_t primID,
                    uniform box3fa &bbox)
{
  // this is actually the boudns of a SPHERE, so MUCH too big! TODO: Fix this ...
  uniform uint8 *uniform diskPtr = geometry->data + geometry->bytesPerDisk*primID;
  uniform float radius = geometry->radius;
  if (geometry->offset_radius >= 0) {
    radius = *((uniform float *)(diskPtr+geometry->offset_radius));
  }
  uniform vec3f center = *((uniform vec3f*)(diskPtr+geometry->offset_center));
  bbox.lower = center-make_vec3f(radius);
  bbox.upper = center+make_vec3f(radius);
}

void Disks_intersect(uniform Disks *uniform geometry,
                     varying Ray &ray,
                     uniform size_t primID)
{
  uniform uint8 *uniform diskPtr = geometry->data + geometry->bytesPerDisk*primID;
  uniform float radius = geometry->radius;
  if (geometry->offset_radius >= 0) {
    radius = *((uniform float *)(diskPtr+geometry->offset_radius));
  }
  uniform vec3f center = *((uniform vec3f*)(diskPtr+geometry->offset_center));
  uniform vec3f normal = *((uniform vec3f*)(diskPtr+geometry->offset_normal));

  float dotDN = dot(ray.dir,normal);
  if (absf(dotDN) < 1e-6f) return;
  
  float t = - dot(ray.org - center,normal) * rcpf(dotDN);
  if (t <= ray.t0 || t >= ray.t)
    return;

  vec3f P = ray.org + t * ray.dir;
  float distP = length(P-center);
  if (distP > radius)
    return;

  ray.primID = primID;
  ray.geomID = geometry->geometry.geomID;
  ray.t = t;
  ray.Ng = normal;
}


export void *uniform Disks_create(void           *uniform cppEquivalent)
{
  uniform Disks *uniform geom = uniform new uniform Disks;
  Geometry_Constructor(&geom->geometry,cppEquivalent,
                       Disks_postIntersect,
                       NULL,0,NULL);
  return geom;
}

export void DisksGeometry_set(void           *uniform _geom,
                              void           *uniform _model,
                              void           *uniform data,
                              int             uniform numDisks,
                              int             uniform bytesPerDisk,
                              float           uniform radius,
                              int             uniform materialID,
                              int             uniform offset_center,
                              int             uniform offset_normal,
                              int             uniform offset_radius,
                              int             uniform offset_materialID)
{
  uniform Disks *uniform geom = (uniform Disks *uniform)_geom;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numDisks);
  
  geom->geometry.model = model;
  geom->geometry.geomID = geomID;
  geom->numDisks = numDisks;
  geom->radius = radius;
  geom->data = (uniform uint8 *uniform)data;
  geom->materialID = materialID;
  geom->bytesPerDisk = bytesPerDisk;

  geom->offset_center     = offset_center;
  geom->offset_normal     = offset_normal;
  geom->offset_radius     = offset_radius;
  geom->offset_materialID = offset_materialID;

  rtcSetUserData(model->embreeSceneHandle,geomID,geom);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&Disks_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&Disks_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCOccludedFuncVarying)&Disks_intersect);
  rtcEnable(model->embreeSceneHandle,geomID);

}
