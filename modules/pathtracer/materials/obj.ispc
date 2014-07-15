/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "./material.ih"
#include "../textures/texture.ih"
#include "../brdfs/transmission.ih"
#include "../brdfs/lambertian.ih"
#include "../brdfs/specular.ih"

struct OBJ
{
  uniform PathTraceMaterial base;
  uniform Texture* map_d;  
  float d; 
  uniform Texture* map_Kd; 
  vec3f Kd;
  uniform Texture* map_Ks; 
  vec3f Ks;
  uniform Texture* map_Ns; 
  float Ns;
  uniform Texture* map_Bump;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void OBJ__shade(const uniform PathTraceMaterial *uniform _THIS,
                const Ray&                  ray,           
                const Medium&               currentMedium, 
                const DifferentialGeometry& dg,            
                uniform CompositedBRDF&     brdfs)         
{
  uniform const OBJ* uniform THIS = (uniform const OBJ* uniform)_THIS;


  /*! transmission */
  float d = THIS->d;  
  if (THIS->map_d) { const vec3f c = THIS->map_d->get(THIS->map_d,dg.st); d *= c.x; }
  if (d < 1.0f) COMPOSITED_BRDF_ADD(brdfs,Transmission,make_vec3f(1.0f-d));
  /*! diffuse component */
  vec3f Kd = mul(d, THIS->Kd);  
  if (THIS->map_Kd) Kd = mul(Kd, THIS->map_Kd->get(THIS->map_Kd,dg.st));  
  if (ne(Kd,make_vec3f(0.0f))) COMPOSITED_BRDF_ADD(brdfs,Lambertian,Kd);

#if 0
  /*! specular exponent */
  float Ns = THIS->Ns;  
  if (THIS->map_Ns) { const vec3f c = THIS->map_Ns->get(THIS->map_Ns,dg.st); Ns *= c.x; }
  
  /*! specular component */
  vec3f Ks = mul(d, THIS->Ks);  
  if (THIS->map_Ks) Ks = mul(Ks, THIS->map_Ks->get(THIS->map_Ks,dg.st));  
  if (ne(Ks,make_vec3f(0.0f))) COMPOSITED_BRDF_ADD(brdfs,Specular,Ks,Ns);
#endif
}

// void OBJ__Destructor(uniform RefCount* uniform _THIS)
// {
//   uniform OBJ* uniform THIS = (uniform OBJ* uniform) _THIS;
//   if (THIS->map_d ) RefCount__DecRef(&THIS->map_d->base);
//   if (THIS->map_Kd) RefCount__DecRef(&THIS->map_Kd->base);
//   if (THIS->map_Ks) RefCount__DecRef(&THIS->map_Ks->base);
//   if (THIS->map_Ns) RefCount__DecRef(&THIS->map_Ns->base);
//   if (THIS->map_Bump) RefCount__DecRef(&THIS->map_Bump->base);
//   Material__Destructor(_THIS);
// }

void OBJ__Constructor(uniform OBJ* uniform THIS,
                      uniform Texture* uniform map_d,  const uniform float d,
                      uniform Texture* uniform map_Kd, const uniform vec3f Kd,
                      uniform Texture* uniform map_Ks, const uniform vec3f Ks,
                      uniform Texture* uniform map_Ns, const uniform float Ns,
                      uniform Texture* uniform map_Bump)
{
  PathTraceMaterial__Constructor(&THIS->base,// OBJ__Destructor,
                                 OBJ__shade,NULL);
  THIS->map_d  = map_d;  THIS->d  = d;  //if (THIS->map_d ) RefCount__IncRef(&map_d->base);
  THIS->map_Kd = map_Kd; THIS->Kd = Kd; //if (this->map_Kd) RefCount__IncRef(&map_Kd->base);
  THIS->map_Ks = map_Ks; THIS->Ks = Ks; //if (THIS->map_Ks) RefCount__IncRef(&map_Ks->base);
  THIS->map_Ns = map_Ns; THIS->Ns = Ns; //if (THIS->map_Ns) RefCount__IncRef(&map_Ns->base);
  THIS->map_Bump = map_Bump; //if (THIS->map_Bump) RefCount__IncRef(&map_Bump->base);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform 
PathTracer_OBJ_create(void* uniform map_d,  const uniform float& d,
                      void* uniform map_Kd, const uniform vec3f& Kd,
                      void* uniform map_Ks, const uniform vec3f& Ks,
                      void* uniform map_Ns, const uniform float& Ns,
                      void* uniform map_Bump)
{
  uniform OBJ* uniform THIS = uniform new uniform OBJ;
  OBJ__Constructor(THIS,
                   (uniform Texture* uniform)map_d,d,
                   (uniform Texture* uniform)map_Kd,Kd,
                   (uniform Texture* uniform)map_Ks,Ks,
                   (uniform Texture* uniform)map_Ns,Ns,
                   (uniform Texture* uniform)map_Bump);
  return THIS;
}
