// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "./Material.ih"
#include "ospray/texture/Texture2D.ih"
#include "../brdfs/Transmission.ih"
#include "../brdfs/Lambertian.ih"
#include "../brdfs/Specular.ih"

struct OBJ
{
  uniform PathTraceMaterial base;
  uniform Texture2D* map_d;
  float d;
  uniform Texture2D* map_Kd;
  vec3f Kd;
  uniform Texture2D* map_Ks;
  vec3f Ks;
  uniform Texture2D* map_Ns;
  float Ns;
  uniform Texture2D* map_Bump;
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
  // textures modify (mul) values, see http://paulbourke.net/dataformats/mtl/

  /*! transmission */
  float d = THIS->d * get1f(THIS->map_d, dg.st, 1.f);
  if (d < 1.0f) COMPOSITED_BRDF_ADD(brdfs,Transmission,make_vec3f(1.0f-d));

  /*! diffuse component */
  vec3f Kd = d * THIS->Kd * get3f(THIS->map_Kd, dg.st, make_vec3f(1.f));
  if (ne(Kd,make_vec3f(0.0f))) COMPOSITED_BRDF_ADD(brdfs,Lambertian,Kd);

  /*! specular exponent */
  float Ns = THIS->Ns * get1f(THIS->map_Ns, dg.st, 1.0f);
  vec3f Ks = d * THIS->Ks * get3f(THIS->map_Ks, dg.st, make_vec3f(1.f));
  if (ne(Ks,make_vec3f(0.0f))) COMPOSITED_BRDF_ADD(brdfs,Specular,Ks,Ns);
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
                      uniform Texture2D* uniform map_d,  const uniform float d,
                      uniform Texture2D* uniform map_Kd, const uniform vec3f Kd,
                      uniform Texture2D* uniform map_Ks, const uniform vec3f Ks,
                      uniform Texture2D* uniform map_Ns, const uniform float Ns,
                      uniform Texture2D* uniform map_Bump)
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
                   (uniform Texture2D* uniform)map_d,   d,
                   (uniform Texture2D* uniform)map_Kd,  Kd,
                   (uniform Texture2D* uniform)map_Ks,  Ks,
                   (uniform Texture2D* uniform)map_Ns,  Ns,
                   (uniform Texture2D* uniform)map_Bump);
  return THIS;
}
