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
#include "ospray/geometry/geometry.ih"
#include "ospray/common/Model.ih"
#include "ospray/transferfunction/TransferFunction.ih"
// this module
#include "MinMaxBVH2.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct AlphaSpheres {
  uniform Geometry geometry; //!< inherited geometry fields

  vec3f *uniform position;
  float *uniform attribute;

  //  uniform Material *uniform *materialList;
  uniform TransferFunction *uniform transferFunction;

  // the min-max BVH over the primitives
  MinMaxBVH2      mmBVH;

  float           radius;
  int32           numSpheres;
};

typedef uniform float uniform_float;

static void AlphaSpheres_postIntersect(uniform Geometry *uniform geometry,
                                       uniform Model *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  uniform AlphaSpheres *uniform THIS = (uniform AlphaSpheres *uniform)geometry;
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

  if (flags & DG_COLOR) {
    // compute base pointer to actual sphere we are intersecting
    foreach_unique(primID in ray.primID) {
      // uniform uint8 *uniform spherePtr
      //   = THIS->data + THIS->bytesPerSphere*((uniform int64)primID);

      // // read sphere members relative to sphere base pointr.
      // uniform float radius = *((uniform float *)(spherePtr+THIS->offset_radius));
      // uniform vec3f center = *((uniform vec3f*)(spherePtr+THIS->offset_center));
      // uniform float attrib = *((uniform float *)(spherePtr+THIS->offset_attribute));

      uniform float attrib = THIS->attribute[primID];

      // normalize attribute to the [0,1] range (by normalizing relative
      // to the attribute range stored in the min max BVH's root node
      const uniform float attrib_lo = THIS->mmBVH.node[0].range_lo;
      const uniform float attrib_hi = THIS->mmBVH.node[0].range_hi;
      attrib = (attrib - attrib_lo) * rcp(attrib_hi - attrib_lo + 1e-10f);

      // compute alpha value from attribute value
      float alpha = THIS->transferFunction->getAlphaForValue(THIS->transferFunction,
                                                             attrib);
      vec3f color = THIS->transferFunction->getColorForValue(THIS->transferFunction,
                                                             attrib);
      dg.color = make_vec4f(color.x,color.y,color.z,alpha);
    }
  }

  dg.Ng = Ng;
  dg.Ns = Ns;
}

void AlphaSpheres_bounds(uniform AlphaSpheres *uniform geometry,
                         uniform size_t primID,
                         uniform box3fa &bbox)
{
  bbox.lower = geometry->mmBVH.node[0].bounds_lo;
  bbox.upper = geometry->mmBVH.node[0].bounds_hi;
}

static
varying bool AlphaSpheres_intersectPrim(void *uniform geomPtr,
                                uniform int primID,
                                varying Ray &ray)
{
  // typecast "implicit THIS" pointer to the proper geometry type
  AlphaSpheres *uniform THIS = (AlphaSpheres *uniform)geomPtr;

  // read sphere members required for intersection test
  uniform float radius = THIS->radius;
  uniform vec3f center = THIS->position[primID];

  // perform first half of intersection test ....
  const vec3f A = center - ray.org;

  const float a = dot(ray.dir,ray.dir);
  const float b = -2.f*dot(ray.dir,A);
  const float c = dot(A,A)-radius*radius;
  
  const float radical = b*b-4.f*a*c;
  if (radical < 0.f) return false;


  // compute second half of intersection test
  const float srad = sqrt(radical);

  const float t_in = (- b - srad) *rcpf(2.f*a);
  const float t_out= (- b + srad) *rcpf(2.f*a);

  float hit_t = 0.f;
  if (t_in > ray.t0 && t_in < ray.t) {
    hit_t = t_in;
  } else if (t_out > ray.t0 && t_out < ray.t) {
    hit_t = t_out;
  }
  else /* miss : */ return false;

  // -------------------------------------------------------
  // do attribute test
  uniform float attrib = THIS->attribute[primID];

  // normalize attribute to the [0,1] range (by normalizing relative
  // to the attribute range stored in the min max BVH's root node
  const uniform float attrib_lo = THIS->mmBVH.node[0].range_lo;
  const uniform float attrib_hi = THIS->mmBVH.node[0].range_hi;
  attrib = (attrib - attrib_lo) * rcp(attrib_hi - attrib_lo);

  // compute alpha value from attribute value
  const float alpha
    = THIS->transferFunction->getAlphaForValue(THIS->transferFunction,attrib);
  if (alpha <= .5f)
    return false;

  // found a hit - store it
  ray.primID = primID;
  ray.geomID = THIS->geometry.geomID;
  ray.t = t_in;
  ray.Ng = ray.org + ray.t*ray.dir - center;
  return true;
}


void AlphaSpheres_intersect(uniform AlphaSpheres *uniform geometry,
                            varying Ray &ray,
                            uniform size_t primID)
{
  MinMaxBVH2_intersect(&geometry->mmBVH,geometry,geometry->transferFunction,
                      &AlphaSpheres_intersectPrim,geometry->numSpheres,ray);
}

void AlphaSpheres_occluded(uniform AlphaSpheres *uniform geometry,
                            varying Ray &ray,
                            uniform size_t primID)
{
  MinMaxBVH2_occluded(&geometry->mmBVH,geometry,geometry->transferFunction,
                      &AlphaSpheres_intersectPrim,geometry->numSpheres,ray);
}


export void *uniform AlphaSpheres_create(void           *uniform cppEquivalent)
{
  uniform AlphaSpheres *uniform geom = uniform new uniform AlphaSpheres;
  Geometry_Constructor(&geom->geometry,cppEquivalent,
                       AlphaSpheres_postIntersect,
                       NULL,0,NULL);
  return geom;
}

export void AlphaSpheres_set(void           *uniform _geom,
                             void           *uniform _model,
                             void           *uniform transferFunction,
                             int64           uniform rootRef,
                             const void     *uniform bvhNode,
                             const uint32   *uniform primID,
                             float           uniform radius,
                             vec3f          *uniform positionData,
                             float          *uniform attributeData,
                             int             uniform numSpheres)
{
  uniform AlphaSpheres *uniform geom = (uniform AlphaSpheres *uniform)_geom;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,1); //numSpheres);
  
  geom->geometry.model = model;
  geom->geometry.geomID = geomID;
  // geom->materialList = (Material **)materialList;
  geom->numSpheres = numSpheres;
  geom->radius = radius;
  geom->position  = positionData;
  geom->attribute = attributeData;

  print("radius %\n",radius);

  geom->mmBVH.rootRef = rootRef;
  geom->mmBVH.node    = (MinMaxBVH2Node*uniform)bvhNode;
  geom->mmBVH.primID  = primID;

  geom->transferFunction  = (TransferFunction *uniform)transferFunction;

  rtcSetUserData(model->embreeSceneHandle,geomID,geom);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&AlphaSpheres_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&AlphaSpheres_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&AlphaSpheres_occluded);
}
