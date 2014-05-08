// ospray
#include "common/math/vec.ih"
#include "common/math/affine.ih"
#include "common/math/bbox.ih"
#include "common/ray.ih"
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


// ISPC bug: can't export pointer to model because instance contains (through N indirections) a function pointer that has a varying parameter ...
//export uniform Instance
export void *uniform InstanceGeometry_create(void *uniform cppE)
{
  uniform Instance *uniform inst = uniform new uniform Instance;
  inst->geometry.cppEquivalent = cppE;

  return inst;
}


