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

void OBJ__shade(const uniform PathTraceMaterial *uniform _self,
                const Ray&                  ray,
                const Medium&               currentMedium,
                const DifferentialGeometry& dg,
                uniform CompositedBRDF&     brdfs)
{
  uniform const OBJ* uniform self = (uniform const OBJ* uniform)_self;
  // textures modify (mul) values, see http://paulbourke.net/dataformats/mtl/

  /*! transmission */
  float d = self->d * get1f(self->map_d, dg.st, 1.f);

  /*! diffuse component */
  vec3f Kd = self->Kd;
  if (self->map_Kd) {
    vec4f Kd_from_map = get4f(self->map_Kd,dg.st);
    Kd = Kd * make_vec3f(Kd_from_map);
    d *= 1.f - Kd_from_map.w;
  }
  Kd = Kd * d * make_vec3f(dg.color);
  if (ne(Kd,make_vec3f(0.0f))) 
    COMPOSITED_BRDF_ADD(brdfs,Lambertian,Kd);

  /*! transmission component */
  if (d < 1.0f) 
    COMPOSITED_BRDF_ADD(brdfs,Transmission,make_vec3f(1.0f-d));

  /*! specular component */
  float Ns = self->Ns * get1f(self->map_Ns, dg.st, 1.0f);
  vec3f Ks = d * self->Ks * get3f(self->map_Ks, dg.st, make_vec3f(1.f));
  if (ne(Ks,make_vec3f(0.0f))) COMPOSITED_BRDF_ADD(brdfs,Specular,Ks,Ns);
}

// void OBJ__Destructor(uniform RefCount* uniform _self)
// {
//   uniform OBJ* uniform self = (uniform OBJ* uniform) _self;
//   if (self->map_d ) RefCount__DecRef(&self->map_d->base);
//   if (self->map_Kd) RefCount__DecRef(&self->map_Kd->base);
//   if (self->map_Ks) RefCount__DecRef(&self->map_Ks->base);
//   if (self->map_Ns) RefCount__DecRef(&self->map_Ns->base);
//   if (self->map_Bump) RefCount__DecRef(&self->map_Bump->base);
//   Material__Destructor(_self);
// }

void OBJ__Constructor(uniform OBJ* uniform self,
                      uniform Texture2D* uniform map_d,  const uniform float d,
                      uniform Texture2D* uniform map_Kd, const uniform vec3f Kd,
                      uniform Texture2D* uniform map_Ks, const uniform vec3f Ks,
                      uniform Texture2D* uniform map_Ns, const uniform float Ns,
                      uniform Texture2D* uniform map_Bump)
{
  PathTraceMaterial__Constructor(&self->base,// OBJ__Destructor,
                                 OBJ__shade,NULL);
  self->map_d  = map_d;  self->d  = d;  //if (self->map_d ) RefCount__IncRef(&map_d->base);
  self->map_Kd = map_Kd; self->Kd = Kd; //if (this->map_Kd) RefCount__IncRef(&map_Kd->base);
  self->map_Ks = map_Ks; self->Ks = Ks; //if (self->map_Ks) RefCount__IncRef(&map_Ks->base);
  self->map_Ns = map_Ns; self->Ns = Ns; //if (self->map_Ns) RefCount__IncRef(&map_Ns->base);
  self->map_Bump = map_Bump; //if (self->map_Bump) RefCount__IncRef(&map_Bump->base);
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
  uniform OBJ* uniform self = uniform new uniform OBJ;
  OBJ__Constructor(self,
                   (uniform Texture2D* uniform)map_d,   d,
                   (uniform Texture2D* uniform)map_Kd,  Kd,
                   (uniform Texture2D* uniform)map_Ks,  Ks,
                   (uniform Texture2D* uniform)map_Ns,  Ns,
                   (uniform Texture2D* uniform)map_Bump);
  return self;
}
