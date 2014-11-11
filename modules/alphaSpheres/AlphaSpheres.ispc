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
#include "ospray/transferfunction/LinearTransferFunction.ih"
// this module
#include "MinMaxBVH2.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

#include "ospray/common/Box.ih"

struct PKDParticle {
  uniform float position[3];
  uniform float attribute;
};

struct PKDGeometry {
  uniform Geometry geometry; //!< inherited geometry fields

  PKDParticle *uniform particle;
  size_t numParticles;
  uint32 *uniform innerNodeInfo;
  size_t numInnerNodes;
  box3f centerBounds;
  box3f sphereBounds;
  float attr_lo, attr_hi;
  uniform int32 activeBinBits;

  uniform TransferFunction *uniform transferFunction;

  float           radius;
};

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

static void PKDGeometry_postIntersect(uniform Geometry *uniform geometry,
                                       uniform Model *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  uniform PKDGeometry *uniform THIS = (uniform PKDGeometry *uniform)geometry;
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

      uniform float attrib = THIS->particle[primID].attribute;

      // normalize attribute to the [0,1] range (by normalizing relative
      // to the attribute range stored in the min max BVH's root node
      const uniform float attrib_lo = THIS->attr_lo;
      const uniform float attrib_hi = THIS->attr_hi;
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

void PKDGeometry_bounds(uniform PKDGeometry *uniform geometry,
                         uniform size_t primID,
                         uniform box3fa &bbox)
{
  bbox.lower = geometry->sphereBounds.lower;
  bbox.upper = geometry->sphereBounds.upper;
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
  attrib = (attrib - attrib_lo) * rcp(attrib_hi - attrib_lo + 1e-10f);

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





inline
varying bool PKDGeometry_intersectPrim(void *uniform geomPtr,
                                       uniform int primID,
                                       varying Ray &ray)
{
  // typecast "implicit THIS" pointer to the proper geometry type
  PKDGeometry *uniform THIS = (PKDGeometry *uniform)geomPtr;

  // read sphere members required for intersection test
  uniform float radius = THIS->radius;
  // uniform vec3f center = (uniform vec3f &)THIS->particle[primID].position;
  uniform vec3f center = make_vec3f(THIS->particle[primID].position[0],
                                    THIS->particle[primID].position[1],
                                    THIS->particle[primID].position[2]);
                                    

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
  uniform float attrib = THIS->particle[primID].attribute;

  // normalize attribute to the [0,1] range (by normalizing relative
  // to the attribute range stored in the min max BVH's root node
  const uniform float attrib_lo = THIS->attr_lo;
  const uniform float attrib_hi = THIS->attr_hi;
  attrib = (attrib - attrib_lo) * rcp(attrib_hi - attrib_lo + 1e-10f);

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


struct PKDStackEntry {
  varying float t_in, t_out;
  uniform size_t nodeID;
};

void PKDGeometry_intersect(uniform PKDGeometry *uniform THIS,
                            varying Ray &ray,
                            uniform size_t primID)
{
  float t_in = ray.t0, t_out = ray.t;
  boxtest(ray,THIS->sphereBounds,t_in,t_out);

  
  if (t_out < t_in) return;
  // print("t % %\n",t_in,t_out);

  PKDStackEntry stack[64];
  uniform int32 stackPtr = 0;

  float org[3], dir[3];
  org[0] = ray.org.x;
  org[1] = ray.org.y;
  org[2] = ray.org.z;
  dir[0] = ray.dir.x;
  dir[1] = ray.dir.y;
  dir[2] = ray.dir.z;

  uniform size_t nodeID = 0;
  while (1) {
    while (1) {
      if (t_out < t_in) break;
      if (nodeID >= THIS->numInnerNodes) {
        // this is a leaf ...
        PKDGeometry_intersectPrim(THIS,nodeID,ray);
        break; // pop
      } else {
        const uniform int32  binBits = THIS->innerNodeInfo[nodeID] & ((1<<30)-1);
        if ((THIS->activeBinBits & binBits) == 0)
          break;
        
        PKDGeometry_intersectPrim(THIS,nodeID,ray);
        t_out = min(t_out,ray.t);
        
        const uniform size_t dim = THIS->innerNodeInfo[nodeID] >> 30;
        const uniform float plane_coord = THIS->particle[nodeID].position[dim];

        const float t_plane_0 = (plane_coord + THIS->radius - org[dim]) * rcp(dir[dim]);
        const float t_plane_1 = (plane_coord - THIS->radius - org[dim]) * rcp(dir[dim]);
        
        float t_in_0 = t_in;
        float t_out_0 = t_out;

        float t_in_1 = t_in;
        float t_out_1 = t_out;
        uniform bool goNode1First = false;
        if (dir[dim] > 0.f) {
          t_out_0 = min(t_out_0,t_plane_0);
          t_in_1 = max(t_in_1,t_plane_1);
        } else {
          t_out_1 = min(t_out_1,t_plane_1);
          t_in_0 = max(t_in_0,t_plane_0);
          goNode1First = true;
        }
        uniform bool goNode0 = any(t_out_0 >= t_in_0);
        uniform bool goNode1 = any(t_out_1 >= t_in_1);
        if (goNode0) {
          if (goNode1) {
            if (goNode1First) {
              unmasked { stack[stackPtr].t_in  = 1e20f; }
              unmasked { stack[stackPtr].t_out = -1e20f; }
              stack[stackPtr].t_in  = t_in_0;
              stack[stackPtr].t_out = t_out_0;
              stack[stackPtr].nodeID = 2*nodeID+1;
              stackPtr++;
              t_in = t_in_1;
              t_out = t_out_1;
              nodeID = 2*nodeID+2;
            } else {
              unmasked { stack[stackPtr].t_in  = 1e20f; }
              unmasked { stack[stackPtr].t_out = -1e20f; }
              stack[stackPtr].t_in  = t_in_1;
              stack[stackPtr].t_out = t_out_1;
              stack[stackPtr].nodeID = 2*nodeID+2;
              stackPtr++;
              t_in = t_in_0;
              t_out = t_out_0;
              nodeID = 2*nodeID+1;
            }
            continue;
          } else {
            t_in = t_in_0;
            t_out = t_out_0;
            nodeID = 2*nodeID+1;
            continue;
          }
        } else {
          if (goNode1) {
            t_in = t_in_1;
            t_out = t_out_1;
            nodeID = 2*nodeID+2;
            continue;
          } else {
            break; // pop
          }
        }
      }
    }
    // TODO: pop
    while (1) {
      if (stackPtr == 0) return;
      --stackPtr;
      t_in = stack[stackPtr].t_in;
      t_out = min(stack[stackPtr].t_out,ray.t);
      if (all(t_in > t_out)) continue;
      nodeID = stack[stackPtr].nodeID;
      break;
    };
  }
}


void PKDGeometry_occluded(uniform PKDGeometry *uniform THIS,
                            varying Ray &ray,
                            uniform size_t primID)
{
  float t_in = ray.t0, t_out = ray.t;
  boxtest(ray,THIS->sphereBounds,t_in,t_out);

  
  if (t_out < t_in) return;
  // print("t % %\n",t_in,t_out);

  PKDStackEntry stack[64];
  uniform int32 stackPtr = 0;

  float org[3], dir[3];
  org[0] = ray.org.x;
  org[1] = ray.org.y;
  org[2] = ray.org.z;
  dir[0] = ray.dir.x;
  dir[1] = ray.dir.y;
  dir[2] = ray.dir.z;

  uniform size_t nodeID = 0;
  while (1) {
    while (1) {
      if (t_out < t_in) break;
      if (nodeID >= THIS->numInnerNodes) {
        // this is a leaf ...
        if (PKDGeometry_intersectPrim(THIS,nodeID,ray))
          return;
        break; // pop
      } else {
        const uniform int32  binBits = THIS->innerNodeInfo[nodeID] & ((1<<30)-1);
        if ((THIS->activeBinBits & binBits) == 0)
          break;
        
        if (PKDGeometry_intersectPrim(THIS,nodeID,ray))
          return;
        t_out = min(t_out,ray.t);
        
        const uniform size_t dim = THIS->innerNodeInfo[nodeID] >> 30;
        const uniform float plane_coord = THIS->particle[nodeID].position[dim];

        const float t_plane_0 = (plane_coord + THIS->radius - org[dim]) * rcp(dir[dim]);
        const float t_plane_1 = (plane_coord - THIS->radius - org[dim]) * rcp(dir[dim]);
        
        float t_in_0 = t_in;
        float t_out_0 = t_out;

        float t_in_1 = t_in;
        float t_out_1 = t_out;
        uniform bool goNode1First = false;
        if (dir[dim] > 0.f) {
          t_out_0 = min(t_out_0,t_plane_0);
          t_in_1 = max(t_in_1,t_plane_1);
        } else {
          t_out_1 = min(t_out_1,t_plane_1);
          t_in_0 = max(t_in_0,t_plane_0);
          goNode1First = true;
        }
        uniform bool goNode0 = any(t_out_0 >= t_in_0);
        uniform bool goNode1 = any(t_out_1 >= t_in_1);
        if (goNode0) {
          if (goNode1) {
            if (goNode1First) {
              unmasked { stack[stackPtr].t_in  = 1e20f; }
              unmasked { stack[stackPtr].t_out = -1e20f; }
              stack[stackPtr].t_in  = t_in_0;
              stack[stackPtr].t_out = t_out_0;
              stack[stackPtr].nodeID = 2*nodeID+1;
              stackPtr++;
              t_in = t_in_1;
              t_out = t_out_1;
              nodeID = 2*nodeID+2;
            } else {
              unmasked { stack[stackPtr].t_in  = 1e20f; }
              unmasked { stack[stackPtr].t_out = -1e20f; }
              stack[stackPtr].t_in  = t_in_1;
              stack[stackPtr].t_out = t_out_1;
              stack[stackPtr].nodeID = 2*nodeID+2;
              stackPtr++;
              t_in = t_in_0;
              t_out = t_out_0;
              nodeID = 2*nodeID+1;
            }
            continue;
          } else {
            t_in = t_in_0;
            t_out = t_out_0;
            nodeID = 2*nodeID+1;
            continue;
          }
        } else {
          if (goNode1) {
            t_in = t_in_1;
            t_out = t_out_1;
            nodeID = 2*nodeID+2;
            continue;
          } else {
            break; // pop
          }
        }
      }
    }
    // TODO: pop
    while (1) {
      if (stackPtr == 0) return;
      --stackPtr;
      t_in = stack[stackPtr].t_in;
      t_out = min(stack[stackPtr].t_out,ray.t);
      if (all(t_in > t_out)) continue;
      nodeID = stack[stackPtr].nodeID;
      break;
    };
  }
}


export void *uniform PKDGeometry_create(void           *uniform cppEquivalent)
{
  uniform PKDGeometry *uniform geom = uniform new uniform PKDGeometry;
  Geometry_Constructor(&geom->geometry,cppEquivalent,
                       PKDGeometry_postIntersect,
                       NULL,0,NULL);
  return geom;
}

export void PKDGeometry_updateTransferFunction(void           *uniform _THIS,
                                               void           *uniform _transferFunction)
{
  PKDGeometry *uniform THIS = (PKDGeometry *uniform)_THIS;
  TransferFunction *uniform transferFunction = (TransferFunction *uniform)_transferFunction;
  THIS->activeBinBits = 0;
  for (uniform int i=0;i<30;i++) {
    uniform vec2f alphaRange = LinearTransferFunction_getMinMaxAlphaInRange(transferFunction,i/30.f,(i+1)/30.f);
    if (alphaRange.y >= .5f)
      THIS->activeBinBits |= (1UL << i);
  }
  // print("#osp:pkd: new range bits: %\n",(void*uniform )THIS->activeBinBits);
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





export void PKDGeometry_set(void           *uniform _geom,
                            void           *uniform _model,
                            void           *uniform transferFunction,
                            float           uniform radius,
                            PKDParticle *uniform particle,
                            uniform size_t numParticles,
                            uint32 *uniform innerNodeInfo,
                            uniform size_t numInnerNodes,
                            uniform box3f &centerBounds,
                            uniform box3f &sphereBounds,
                            uniform float attr_lo, uniform float attr_hi)
{
  uniform PKDGeometry *uniform geom = (uniform PKDGeometry *uniform)_geom;
  uniform Model *uniform model = (uniform Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,1); //numSpheres);
  
  geom->geometry.model = model;
  geom->geometry.geomID = geomID;
  geom->radius = radius;
  geom->particle  = particle;
  geom->numParticles  = numParticles;
  geom->innerNodeInfo  = innerNodeInfo;
  geom->numInnerNodes = numInnerNodes;
  geom->centerBounds        = centerBounds;
  geom->sphereBounds        = sphereBounds;
  geom->attr_lo = attr_lo;
  geom->attr_hi = attr_hi;

  geom->activeBinBits = 0xffffffff;

  geom->transferFunction  = (TransferFunction *uniform)transferFunction;

  rtcSetUserData(model->embreeSceneHandle,geomID,geom);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&PKDGeometry_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&PKDGeometry_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&PKDGeometry_occluded);
}
