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
#include "ospray/texture/TextureParam.ih"
#include "../brdfs/Transmission.ih"
#include "../brdfs/Lambertian.ih"
#include "../brdfs/Specular.ih"

struct OBJ
{
  uniform PathTraceMaterial base;
  uniform TextureParam map_d;
  float d;
  uniform TextureParam map_Kd;
  vec3f Kd;
  uniform TextureParam map_Ks;
  vec3f Ks;
  uniform TextureParam map_Ns;
  float Ns;
  uniform TextureParam map_Bump;
  linear2f rot_Bump; // just the inverse of rotational/mirror part (must be orthonormal) of tc xfrom
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void OBJ__shade(const uniform PathTraceMaterial *uniform _self,
                const Ray&                  ray,
                const Medium&               currentMedium,
                const DifferentialGeometry& dg,
                uniform CompositedBRDF&     brdfs,
                vec3f& shadingNormal)
{
  uniform const OBJ* uniform self = (uniform const OBJ* uniform)_self;
  // textures modify (mul) values, see http://paulbourke.net/dataformats/mtl/

  /*! transmission */
  float d = self->d * get1f(self->map_d, dg.st, 1.f);

  /*! diffuse component */
  vec3f Kd = self->Kd;
  if (valid(self->map_Kd)) {
    vec4f Kd_from_map = get4f(self->map_Kd,dg.st);
    Kd = Kd * make_vec3f(Kd_from_map);
    d *= Kd_from_map.w;
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

  /*! normal map */
  if (valid(self->map_Bump)) {
    // get normal from texture
    vec3f localNormal = get3f(self->map_Bump, dg.st, make_vec3f(.5f, .5f, 1.f)) * 2.f - 1.f;
    // rotate in 2D (tangent space) to account for tc transformations
    vec2f rotNormal = self->rot_Bump * make_vec2f(localNormal.x, localNormal.y);
    localNormal.x = rotNormal.x; localNormal.y = rotNormal.y;
    // transform to world space and align to tangents/texture coordinates
    linear3f f = make_LinearSpace3f(normalize(dg.dPds), normalize(dg.dPdt), dg.Ng);
    shadingNormal = f * localNormal;
    // in general f is not ortho-normal, thus need to re-normalize
    shadingNormal = normalize(shadingNormal);
  }
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void PathTracer_OBJ_set(
    void* uniform _self,
    void* uniform map_d,  const uniform affine2f &xform_d,  const uniform float d,
    void* uniform map_Kd, const uniform affine2f &xform_Kd, const uniform vec3f &Kd,
    void* uniform map_Ks, const uniform affine2f &xform_Ks, const uniform vec3f &Ks,
    void* uniform map_Ns, const uniform affine2f &xform_Ns, const uniform float Ns,
    void* uniform map_Bump, const uniform affine2f &xform_Bump, const uniform linear2f &rot_Bump)
{
  uniform OBJ* uniform self = (uniform OBJ* uniform)_self;
  self->map_d  = make_TextureParam((uniform Texture2D*)map_d,  xform_d);  self->d  = d;
  self->map_Kd = make_TextureParam((uniform Texture2D*)map_Kd, xform_Kd); self->Kd = Kd;
  self->map_Ks = make_TextureParam((uniform Texture2D*)map_Ks, xform_Ks); self->Ks = Ks;
  self->map_Ns = make_TextureParam((uniform Texture2D*)map_Ns, xform_Ns); self->Ns = Ns;
  self->map_Bump = make_TextureParam((uniform Texture2D*)map_Bump, xform_Bump); self->rot_Bump = rot_Bump;
}

void OBJ__Constructor(uniform OBJ* uniform self)
{
  PathTraceMaterial__Constructor(&self->base, OBJ__shade, NULL);

  uniform affine2f xform = make_AffineSpace2f_identity();

  PathTracer_OBJ_set(self,
                     NULL, xform, 1.0f,
                     NULL, xform, make_vec3f(0.8f),
                     NULL, xform, make_vec3f(0.f),
                     NULL, xform, 2.0f,
                     NULL, xform, make_LinearSpace2f_identity());
}

export void* uniform PathTracer_OBJ_create()
{
  uniform OBJ* uniform self = uniform new uniform OBJ;
  OBJ__Constructor(self);
  return self;
}
