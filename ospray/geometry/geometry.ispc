#include "geometry.ih"

export void Geometry_setMaterial(void *uniform _geo, void *uniform _mat)
{
  uniform Geometry *uniform geo = (uniform Geometry *uniform)_geo;
  geo->material = (uniform Material *uniform)_mat;
}

//! constructor for ispc-side Geometry object
static void Geometry_Constructor(uniform Geometry *uniform geometry,
                                 void *uniform cppEquivalent,
                                 uniform Model *uniform model,
                                 uniform int32  geomID,
                                 uniform Material *uniform material
                                 )
{
  geometry->cppEquivalent = cppEquivalent;
  geometry->model    = model;
  geometry->material = material;
  geometry->geomID   = geomID;
}

//! destructor for ispc-side Geometry object
extern void Geometry_Destructor(uniform Geometry *uniform mesh)
{
}


