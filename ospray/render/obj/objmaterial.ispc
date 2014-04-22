#include "objmaterial.ih"

export void *uniform OBJMaterial_create(void *uniform cppE)
{
  uniform OBJMaterial *uniform mat = uniform new uniform OBJMaterial;
  mat->base.cppEquivalent = cppE;
  return mat;
}

export void OBJMaterial_destroy(void *uniform _mat)
{
  uniform OBJMaterial *uniform mat = (uniform OBJMaterial *uniform)_mat;
  delete(mat);
}

export void OBJMaterial_set(void *uniform _mat, 
                            const uniform vec3f &Kd,
                            const uniform vec3f &Ks,
                            const uniform float &Ns,
                            const uniform float &d
                            )
{
  // print("created ispc-side obj material (kd=% % %)\n",
  //       Kd.x,Kd.y,Kd.z);
  uniform OBJMaterial *uniform mat = (uniform OBJMaterial *uniform)_mat;
  mat->Kd = Kd;
  mat->Ks = Ks;
  mat->Ns = Ns;
  mat->d  = d;
}
