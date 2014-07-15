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

struct RBF {
  vec3f pos;
  float coeff;
};

struct Node {
  int beginRBFs;
  int endRBFs;
  int upperNodeID;
  int lowerNodeID;
  int materialID;

  // accel plane:
  vec3f center;
  vec3f normal;
  float lower;
  float upper;
};

struct RBFTree {
  uniform Geometry inherited; //!< inherited geometry fields
  uniform RBF *uniform rbf;
  uniform Node *uniform node;
  box3f bounds;
  float scale;
};

struct Range {
  float begin; 
  float end;
};

inline bool isEmpty(Range r) { return r.begin >= r.end; }

static void RBFTree_postIntersect(uniform Geometry *uniform geometry,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  uniform RBFTree *uniform THIS = (uniform RBFTree *uniform)geometry;
  dg.geometry = geometry;
  dg.material = 0;
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
  dg.Ng = Ng;
  dg.Ns = Ns;
}

void RBFTree_bounds(uniform RBFTree *uniform geometry,
                    uniform size_t primID,
                    uniform box3fa &bbox)
{
  print("bounds %\n",geometry);
  bbox = make_box3fa(geometry->bounds);
  print("% % %\n",bbox.lower.x,bbox.lower.y,bbox.lower.z);
  print("% % %\n",bbox.upper.x,bbox.upper.y,bbox.upper.z);
}


//! performs a ray-box intersection test, and returns intersection interval [t0,t1]
inline void boxTest(varying Range &range,
                    const varying Ray &ray,
                    const uniform box3f &box
                    )
{
  const vec3f rdir = rcp(ray.dir);
  const vec3f mins = rdir * (box.lower - ray.org); //mul(sub(box.lower, ray.org), rdir);
  const vec3f maxs = rdir * (box.upper - ray.org); //mul(sub(box.upper, ray.org), rdir);
  
  range.begin = max(max(ray.t0, 
                        min(mins.x,maxs.x)),
                    max(min(mins.y,maxs.y),
                        min(mins.z,maxs.z)));
  
  range.end = min(min(ray.t, 
                      max(mins.x,maxs.x)),
                  min(max(mins.y,maxs.y),
                      max(mins.z,maxs.z)));
}

inline float gaussian(const float _r)
{
  const float c = 0.1f;
  float r = _r * 0.01f;
  return rcpf(sqrtf(r*r+c*c));
}

inline float thinPlate(const float r)
{
  const float c = 0.1f;
  return r*r*logf(r+c);
}

inline float computeDistance(uniform RBFTree *uniform geom,
                             uniform Node *uniform node, 
                             const vec3f p)
{
  float dist = 0.f;
  // print("range % %\n",node->beginRBFs,node->endRBFs);
  for (uniform int i=node->beginRBFs;i<node->endRBFs;i++) {
    float r = length(geom->rbf[i].pos-p);
    // print("r %\n",r);
    dist += geom->rbf[i].coeff * thinPlate(r);
    // dist += geom->rbf[i].coeff * gaussian(r);
  }
  return dist;
}

inline bool sameSide(float a, float b)
{ return a*b >= 0.f; }

#define STACK_DEPTH 32
               
/*! stack of all 'parent' nodes of the current recursive traversal */      
struct State {
  varying Ray *uniform ray;
  varying Range range;

  uniform Node *uniform clipStackNode[STACK_DEPTH];
  uniform float clipStackSign[STACK_DEPTH];
  uniform int clipStackDepth;
  uniform RBFTree *uniform geom;
};

inline void clipAccelPlane(varying Range &range, const varying Ray &ray, 
                           uniform Node *uniform node)
{
#if 0
  const float rcpCosND = rcp(dot(ray.dir,node->normal));
  const float t_plane_lo
    = (node->lower + dot(node->center-ray.org,node->normal)) * rcpCosND;
  const float t_plane_hi
    = (node->upper + dot(node->center-ray.org,node->normal)) * rcpCosND;

  range.begin = max(range.begin,min(t_plane_lo,t_plane_hi));
  range.end   = min(range.end,  max(t_plane_lo,t_plane_hi));
#endif
}

inline bool intersectSurface(uniform State &state,
                             uniform Node *uniform node, float &t)
{
  Range range = state.range;
  clipAccelPlane(range,*state.ray,node);
  if (isEmpty(range)) return false;
  
  const uniform int numSegments = 16;
  const float dt = (range.end - range.begin)*(1.f/(float)numSegments);
  
  float t0 = range.begin;
  float t1 = t0+dt;
  
  float f0 = computeDistance(state.geom,node,state.ray->org + t0 * state.ray->dir);
  float f1 = 0.f;
  
  for (uniform int segID=0;segID<numSegments;segID++,t0+=dt,t1+=dt,f0=f1) {
    f1 = computeDistance(state.geom,node,state.ray->org + t1 * state.ray->dir);
    if (sameSide(f0,f1)) continue;
    
    while ((t1 - t0) > 1e-4f*state.geom->scale) {
      float t_mid = 0.5f*(t0+t1);
      // float t_mid = t0 - f0/(f1-f0)*(t1-t0);
      float f_mid = computeDistance(state.geom,node,state.ray->org+t_mid *state.ray->dir);
      if (sameSide(f_mid,f1)) {
        f1 = f_mid;
        t1 = t_mid;
      } else {
        f0 = f_mid;
        t0 = t_mid;
      }
    }
    t = 0.5f*(t0+t1);
    return true;
  }
  return false;
}


// void traverseRec(uniform RBFTree *uniform geom,
//                  uniform int nodeID,
//                  varying Ray &ray,
//                  varying float t0, varying float t1)
// {
//   uniform Node *uniform node = &geom->node[nodeID];
//   float t;
//   vec3f N;
//   /*! check if it has _any_
//   bool hadAnyIsec
//   if (intersectSurface(geom,node,ray,t0,t1,t,N)) {
//     if (t < ray.t) {
//       ray.geomID = geom->inherited.geomID;
//       ray.primID = nodeID;
//       ray.t = t;
//       ray.Ng = N;
//       t1 = t;
//     }
//     if (computeDistance(geom,node,ray.org) >= 0.f) {
//       if (node->leftNodeID >= 0)
//         traverseRec(geom,node->leftNodeID,ray,t0,t1);
//     } else {
//       if (node->rightNodeID >= 0)
//         traverseRec(geom,node->rightNodeID,ray,t0,t1);
//     }
//   } else {
//     if (computeDistance(geom,node,ray.org) >= 0.f) {
//       if (node->leftNodeID >= 0)
//         traverseRec(geom,node->leftNodeID,ray,t0,t1);
//       if (node->rightNodeID >= 0)
//         traverseRec(geom,node->rightNodeID,ray,t0,t1);
//     } else {
//       if (node->rightNodeID >= 0)
//         traverseRec(geom,node->rightNodeID,ray,t0,t1);
//       if (node->leftNodeID >= 0)
//         traverseRec(geom,node->leftNodeID,ray,t0,t1);
//     }
//   }
// }
  
inline bool clippedAway(const vec3f P,
                        const uniform State &state)
{
  // return false;

  for (uniform int i=0;i<state.clipStackDepth;i++) 
    if (!sameSide(state.clipStackSign[i],
                  computeDistance(state.geom,state.clipStackNode[i],P)))
      return true;
  return false;
}

void traverseRec(uniform State &state,uniform Node *uniform node)
{
  float t_surf;
  bool foundIntersection = intersectSurface(state,node,t_surf);
  // if (foundIntersection)
  //   print("found isec @%\n",t_surf);

  if (foundIntersection && !clippedAway(state.ray->org+t_surf*state.ray->dir,state)) {
    // print("found isec @%\n",t_surf);
    state.ray->geomID = state.geom->inherited.geomID;
    state.ray->primID = node - state.geom->node;
    state.ray->t = t_surf;
    state.range.end = t_surf;
  }

  // for starters, don't worry about ordering ...
  state.clipStackNode[state.clipStackDepth] = node;
  state.clipStackDepth++;

  if (node->upperNodeID >= 0) {
    state.clipStackSign[state.clipStackDepth-1] = +1.f;
    traverseRec(state,state.geom->node+node->upperNodeID);
  }
  if (node->lowerNodeID >= 0) {
    state.clipStackSign[state.clipStackDepth-1] = -1.f;
    traverseRec(state,state.geom->node+node->lowerNodeID);
  }

  state.clipStackDepth--;

  // // iw: merge this with computeDistance(t0) in intersectSurface...
  // bool orgIsOnFrontSide = computeDistance(geom,node,ray.org) > 0;
  

  // if (any(orgIsOnFrontSide)) { /* iw: note the 'any' - what if have
  //                                 multiple orgs? on differentet
  //                                 sides!? */
  //   // origin on front side -> do front side first -> _push_ back side first
  //   if (node->backNodeID >= 0
  //       //&& any(!foundIntersection)
  //         ) {
  //       nodeStack[stackDepth++] = geom->node+node->backNodeID;
  //       clipStack[clipStack].node = geom->node;
  //       clipStack[clipStack].validSide = -1.f;
  //       clipStack++;
  //     }
  //     if (node->frontNodeID >= 0) {
  //       nodeStack[stackDepth++] = geom->node+node->frontNodeID;
  //       clipStack[clipStack].node = geom->node;
  //       clipStack[clipStack].validSide = -1.f;
  //       clipStack++;
  //     }
  //   } else {
  //     if (node->frontNodeID >= 0
  //         //&& any(!foundIntersection)
  //         ) {
  //       nodeStack[stackDepth++] = geom->node+node->frontNodeID;
  //       clipStack[clipStack].node = geom->node;
  //       clipStack[clipStack].validSide = -1.f;
  //       clipStack++;
  //     }
  //     if (node->backNodeID >= 0) {
  //       nodeStack[stackDepth++] = geom->node+node->backNodeID;
  //       clipStack[clipStack].node = geom->node;
  //       clipStack[clipStack].validSide = -1.f;
  //       clipStack++;
  //     }
  //   }
}

void RBFTree_intersect(uniform RBFTree *uniform geom,
                       varying Ray &ray,
                       uniform size_t primID)
{
  uniform State state;
  state.ray = &ray;
  state.geom = geom;
  boxTest(state.range,*state.ray,geom->bounds);
  if (isEmpty(state.range)) return;

  state.clipStackDepth = 0;
  traverseRec(state,geom->node);
  if (ray.geomID >= 0) {
    vec3f P = ray.org + ray.t * ray.dir;
    foreach_unique(primID in ray.primID) {
      uniform Node *uniform node = geom->node + primID;
      float dt = 10.1f;
      ray.Ng.x
        = computeDistance(geom,node,make_vec3f(P.x+dt,P.y,P.z))
        - computeDistance(geom,node,make_vec3f(P.x-dt,P.y,P.z));
      ray.Ng.y
        = computeDistance(geom,node,make_vec3f(P.x,P.y+dt,P.z))
        - computeDistance(geom,node,make_vec3f(P.x,P.y-dt,P.z));
      ray.Ng.z
        = computeDistance(geom,node,make_vec3f(P.x,P.y,P.z+dt))
        - computeDistance(geom,node,make_vec3f(P.x,P.y,P.z-dt));
    }
  }
}


//   Range range;

//   uniform ClipStack clipStack;
//   clipStack.depth = 0;

//   uniform Node *uniform nodeStack[STACK_DEPTH];
//   nodeStack[0] = geom->node;
//   uniform int stackDepth = 1;
//   uniform int clipStackDepth = 0;

//   while (1) {
//     uniform Node *uniform node = nodeStack[--stackDepth];
//     float t_surf;
//     // vec3f N;
//     bool foundIntersection = intersectSurface(geom,node,ray,range,t_surf);
//     if (foundIntersection && !clippedAway(ray.org+t_surf*ray.dir,
//                                           geom,clipStack,clipStackDepth)) {
//       ray.geomID = geom->inherited.geomID;
//       ray.primID = node - geom->node;
//       ray.t = t_surf;
//       range.end = t_surf;
//     }

//   }
// // #else
// //   uniform Node *uniform node = geom->node;
// //   float t;
// //   vec3f N;
// //   if (intersectSurface(geom,node,ray,t0,t1,t,N)) {
// //     ray.geomID = geom->inherited.geomID;
// //     ray.primID = primID;
// //     ray.Ng = N;
// //     ray.t = t0;
// //   }
// // #endif
// }


export void *uniform RBFTree_create(void           *uniform cppEquivalent)
{
  uniform RBFTree *uniform geom = uniform new uniform RBFTree;
  Geometry_Constructor(&geom->inherited,cppEquivalent,
                       RBFTree_postIntersect,
                       NULL,0,NULL);
  print("created geom %\n",geom);
  return geom;
}

export void RBFTreeGeometry_set(void           *uniform _geom,
                                void           *uniform _model,
                                void           *uniform _rbf,
                                void           *uniform _node,
                                uniform box3f  &bounds)
{
  uniform RBFTree *uniform geom = (uniform RBFTree *uniform)_geom;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  
  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,1);
  print("Got geomid %\n",geomID);

  geom->inherited.model = model;
  geom->inherited.geomID = geomID;
  geom->rbf = (uniform RBF *uniform)_rbf;
  geom->node = (uniform Node *uniform)_node;

  // print("addr of node array %\n",_node);
  // print("RBF range of node[0] : % %\n",geom->node[0].beginRBFs,geom->node[0].endRBFs);

  geom->bounds = bounds;
  geom->scale = length(bounds.upper - bounds.lower);

  rtcSetUserData(model->embreeSceneHandle,geomID,geom);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&RBFTree_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&RBFTree_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                         (uniform RTCOccludedFuncVarying)&RBFTree_intersect);
  rtcEnable(model->embreeSceneHandle,geomID);
}
