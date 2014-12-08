/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// ospray
#include "ospray/math/vec.ih"
#include "ospray/math/affine.ih"
#include "ospray/math/bbox.ih"
#include "ospray/common/Ray.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/common/Model.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Instance {
  uniform Geometry geometry; //!< inherited geometry fields
  uniform Model *uniform model; //!< instanced model
  AffineSpace3f          xfm;   //!< instantiation matrix
  AffineSpace3f          rcp_xfm;   //!< rcp(instantiation matrix)
};



static void Instance_postIntersect(uniform Geometry *uniform _THIS,
                                   uniform Model *uniform parentModel,
                                   varying DifferentialGeometry &dg,
                                   const varying Ray &ray,
                                   uniform int64 flags)
{
  // print("instance: postintersect! primID=%\n",ray.primID);
  uniform Instance *uniform THIS = (uniform Instance *uniform)_THIS;
  uniform Model *uniform instancedModel = 
    (uniform Model *uniform)THIS->model;
  foreach_unique(geomID in ray.geomID) {
    // print("instanced geomID/prinID/instID %/%/%\n",geomID,ray.primID,ray.instID);
    uniform Geometry *uniform instancedGeometry = 
      (uniform Geometry *uniform)instancedModel->geometry[geomID];
    instancedGeometry->postIntersect(instancedGeometry,instancedModel,
                                     dg,ray,flags);
    // print("instnaced geom %\n",instancedGeom);
  }
  // print("instnace xml.l.vx\n %\n %\n %\n",THIS->xfm.l.vx.x,THIS->xfm.l.vx.y,THIS->xfm.l.vx.z);
  // print("instnace xml.l.vy\n %\n %\n %\n",THIS->xfm.l.vy.x,THIS->xfm.l.vy.y,THIS->xfm.l.vy.z);
  // print("instnace xml.l.vz\n %\n %\n %\n",THIS->xfm.l.vz.x,THIS->xfm.l.vz.y,THIS->xfm.l.vz.z);
  // print("instance.postint\n %\n %\n %\n",dg.Ns.x,dg.Ns.y,dg.Ns.z);
  dg.Ns = xfmVector(THIS->xfm.l,dg.Ns);
  // print("-> new dg.ns\n %\n %\n %\n",dg.Ns.x,dg.Ns.y,dg.Ns.z);
  dg.Ng = xfmVector(THIS->xfm.l,dg.Ng);
}

// ISPC bug: can't export pointer to model because instance contains (through N indirections) a function pointer that has a varying parameter ...
//export uniform Instance
export void *uniform InstanceGeometry_create(void *uniform cppE)
{
  uniform Instance *uniform inst = uniform new uniform Instance;
  inst->geometry.cppEquivalent = cppE;
  inst->geometry.postIntersect = Instance_postIntersect;

  return inst;
}

export void InstanceGeometry_set(void *uniform _THIS, 
                                 const uniform AffineSpace3f &xfm,
                                 const uniform AffineSpace3f &rcp_xfm,
                                 void *uniform _model)
{
  uniform Instance *uniform THIS = (uniform Instance *uniform)_THIS;
  THIS->model = (uniform Model *uniform)_model;
  // THIS->xfm = make_AffineSpace3f(make_vec3f(1,0,0),
  //                                make_vec3f(0,1,0),
  //                                make_vec3f(0,0,1),
  //                                make_vec3f(0,0,0));
  THIS->xfm = xfm;
  THIS->rcp_xfm = xfm;
}
