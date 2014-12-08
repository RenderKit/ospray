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
#include "ospray/common/Ray.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/common/Model.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Spheres {
  uniform Geometry geometry; //!< inherited geometry fields

  uniform uint8 *uniform data;
  uniform Material *uniform *materialList;

  float           radius;
  int             materialID;
  int             offset_center;
  int             offset_radius;
  int             offset_materialID;
  int32           numSpheres;
  int32           bytesPerSphere;
};

typedef uniform float uniform_float;

static void Spheres_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform Spheres *uniform THIS = (uniform Spheres *uniform)geometry;
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
    const uniform int32 primsPerPage = (1024*1024*128);
    if (any(ray.primID >= primsPerPage )) {
      const int primPageID  = ray.primID / primsPerPage;
      const int localPrimID = ray.primID % primsPerPage;
      foreach_unique(primPage in primPageID) {
        uniform uint8 *uniform pagePtr = THIS->data + (((int64)primPage) * primsPerPage * THIS->bytesPerSphere);
        uniform uint8 *spherePtr = 
          pagePtr + THIS->bytesPerSphere*localPrimID;
        dg.materialID = *((uniform uint32 *varying)(spherePtr+THIS->offset_materialID));
        if (THIS->materialList) {
          dg.material = THIS->materialList[dg.materialID];
        }
      }
    } else
      {
        uniform uint8 *spherePtr = 
          THIS->data + THIS->bytesPerSphere*ray.primID;
        dg.materialID = *((uniform uint32 *varying)(spherePtr+THIS->offset_materialID));
        if (THIS->materialList) {
          dg.material = THIS->materialList[dg.materialID];
        }
      }
  }
  dg.Ng = Ng;
  dg.Ns = Ns;
}

void Spheres_bounds(uniform Spheres *uniform geometry,
                    uniform size_t primID,
                    uniform box3fa &bbox)
{
  uniform uint8 *uniform spherePtr = geometry->data + geometry->bytesPerSphere*((uniform int64)primID);
  uniform float radius = geometry->radius;
  if (geometry->offset_radius >= 0) {
    radius = *((uniform float *)(spherePtr+geometry->offset_radius));
  }
  uniform vec3f center = *((uniform vec3f*)(spherePtr+geometry->offset_center));
  bbox.lower = center-make_vec3f(radius);
  bbox.upper = center+make_vec3f(radius);
}

void Spheres_intersect(uniform Spheres *uniform geometry,
                       varying Ray &ray,
                       uniform size_t primID)
{
  uniform uint8 *uniform spherePtr = geometry->data + geometry->bytesPerSphere*((uniform int64)primID);
  uniform float radius = geometry->radius;
  if (geometry->offset_radius >= 0) {
    radius = *((uniform float *)(spherePtr+geometry->offset_radius));
  }
  uniform vec3f center = *((uniform vec3f*)(spherePtr+geometry->offset_center));
  const vec3f A = center - ray.org;

  const float a = dot(ray.dir,ray.dir);
  const float b = -2.f*dot(ray.dir,A);
  const float c = dot(A,A)-radius*radius;
  
  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return;

  const float srad = sqrt(radical);

  const float t_in = (- b - srad) *rcpf(2.f*a);
  const float t_out= (- b + srad) *rcpf(2.f*a);

  if (t_in > ray.t0 && t_in < ray.t) {
    ray.primID = primID;
    ray.geomID = geometry->geometry.geomID;
    ray.t = t_in;
    ray.Ng = ray.org + ray.t*ray.dir - center;
    return;
  } else if (t_out > ray.t0 && t_out < ray.t) {
    ray.primID = primID;
    ray.geomID = geometry->geometry.geomID;
    ray.t = t_out;
    ray.Ng = ray.org + ray.t*ray.dir - center;
    return;
  }
  return;
}


export void *uniform Spheres_create(void           *uniform cppEquivalent)
{
  uniform Spheres *uniform geom = uniform new uniform Spheres;
  Geometry_Constructor(&geom->geometry,cppEquivalent,
                       Spheres_postIntersect,
                       NULL,0,NULL);
  return geom;
}

export void SpheresGeometry_set(void           *uniform _geom,
                                void           *uniform _model,
                                void           *uniform data,
                                void           *uniform materialList,
                                int             uniform numSpheres,
                                int             uniform bytesPerSphere,
                                float           uniform radius,
                                int             uniform materialID,
                                int             uniform offset_center,
                                int             uniform offset_radius,
                                int             uniform offset_materialID)
{
  uniform Spheres *uniform geom = (uniform Spheres *uniform)_geom;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numSpheres);
  
  geom->geometry.model = model;
  geom->geometry.geomID = geomID;
  geom->materialList = (Material **)materialList;
  geom->numSpheres = numSpheres;
  geom->radius = radius;
  geom->data = (uniform uint8 *uniform)data;
  geom->materialID = materialID;
  geom->bytesPerSphere = bytesPerSphere;

  geom->offset_center     = offset_center;
  geom->offset_radius     = offset_radius;
  geom->offset_materialID = offset_materialID;

  rtcSetUserData(model->embreeSceneHandle,geomID,geom);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&Spheres_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&Spheres_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCOccludedFuncVarying)&Spheres_intersect);
  rtcEnable(model->embreeSceneHandle,geomID);

}
