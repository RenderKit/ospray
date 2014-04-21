#include "objmaterial.ih"

export void *uniform OBJMaterial_create(void *uniform foo)
{
  uniform OBJMaterial *uniform mat = uniform new uniform OBJMaterial;
  mat->base.cppEquivalent = foo;
  return mat;
}

export void OBJMaterial_set(void *uniform _mat, 
                            const uniform vec3f &Kd,
                            const uniform vec3f &Ks,
                            const uniform float &Ns,
                            const uniform float &d
                            )
{
  uniform OBJMaterial *uniform mat = (uniform OBJMaterial *uniform)_mat;
  mat->Kd = Kd;
  mat->Kd = Ks;
  mat->Ns = Ns;
  mat->d  = d;
}
