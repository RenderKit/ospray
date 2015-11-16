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
#include "../bsdfs/MultiBSDF.ih"
#include "../bsdfs/Transmission.ih"
#include "../bsdfs/Lambert.ih"
#include "../bsdfs/Specular.ih"

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

const uniform BSDF* uniform OBJ_getBSDF(const uniform PathTraceMaterial* uniform material,
                                        uniform ShadingContext* uniform ctx,
                                        const DifferentialGeometry& dg,
                                        const Ray& ray,
                                        const Medium& currentMedium)
{
  uniform const OBJ* uniform self = (uniform const OBJ* uniform)material;
  uniform BSDF* uniform bsdf = MultiBSDF_create(ctx);
  // textures modify (mul) values, see http://paulbourke.net/dataformats/mtl/

  /*! normal map */
  vec3f shadingNormal;
  if (self->map_Bump) {
    linear3f f = make_LinearSpace3f(normalize(dg.dPds), normalize(dg.dPdt), dg.Ng);
    shadingNormal = f * (get3f(self->map_Bump, dg.st, make_vec3f(.5f, .5f, 1.f)) * 2.f - 1.f);
    // in general f is not ortho-normal, thus need to re-normalize
    shadingNormal = normalize(shadingNormal);
  } else {
    shadingNormal = dg.Ns;
  }

  varying linear3f* uniform shadingFrame = LinearSpace3f_create(ctx, frame(shadingNormal));

  /*! transmission */
  float d = self->d * get1f(self->map_d, dg.st, 1.f);

  /*! diffuse component */
  vec3f Kd = self->Kd;
  if (self->map_Kd) {
    vec4f Kd_from_map = get4f(self->map_Kd,dg.st);
    Kd = Kd * make_vec3f(Kd_from_map);
    d *= Kd_from_map.w;
  }
  Kd = Kd * d * make_vec3f(dg.color);
  if (ne(Kd,make_vec3f(0.0f)))
    MultiBSDF_add(bsdf, Lambert_create(ctx, shadingFrame, Kd));

  /*! transmission component */
  //if (d < 1.0f)
  //  COMPOSITED_BSDF_ADD(brdfs,Transmission,make_vec3f(1.0f-d));

  /*! specular component */
  float Ns = self->Ns * get1f(self->map_Ns, dg.st, 1.0f);
  vec3f Ks = d * self->Ks * get3f(self->map_Ks, dg.st, make_vec3f(1.f));
  if (ne(Ks,make_vec3f(0.0f)))
    MultiBSDF_add(bsdf, Specular_create(ctx, shadingFrame, Ks, Ns));

  return bsdf;
}

void OBJ_Constructor(uniform OBJ* uniform self,
                     uniform Texture2D* uniform map_d,  const uniform float d,
                     uniform Texture2D* uniform map_Kd, const uniform vec3f Kd,
                     uniform Texture2D* uniform map_Ks, const uniform vec3f Ks,
                     uniform Texture2D* uniform map_Ns, const uniform float Ns,
                     uniform Texture2D* uniform map_Bump)
{
  PathTraceMaterial_Constructor(&self->base, OBJ_getBSDF, NULL);
  self->map_d  = map_d;  self->d  = d;
  self->map_Kd = map_Kd; self->Kd = Kd;
  self->map_Ks = map_Ks; self->Ks = Ks;
  self->map_Ns = map_Ns; self->Ns = Ns;
  self->map_Bump = map_Bump;
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
  OBJ_Constructor(self,
                  (uniform Texture2D* uniform)map_d,   d,
                  (uniform Texture2D* uniform)map_Kd,  Kd,
                  (uniform Texture2D* uniform)map_Ks,  Ks,
                  (uniform Texture2D* uniform)map_Ns,  Ns,
                  (uniform Texture2D* uniform)map_Bump);
  return self;
}
