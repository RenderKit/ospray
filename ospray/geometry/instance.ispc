// ospray
#include "ospray/math/vec.ih"
#include "ospray/math/affine.ih"
#include "ospray/math/bbox.ih"
#include "ospray/common/ray.ih"
#include "ospray/geometry/geometry.ih"
#include "ospray/common/model.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Instance {
  uniform Geometry geometry; //!< inherited geometry fields
  uniform Model *uniform model; //!< instanced model
  AffineSpace3f          xfm;   //!< instantiation matrix
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
                                 void *uniform _model)
{
  uniform Instance *uniform THIS = (uniform Instance *uniform)_THIS;
  THIS->model = (uniform Model *uniform)_model;
  THIS->xfm = xfm;
}
