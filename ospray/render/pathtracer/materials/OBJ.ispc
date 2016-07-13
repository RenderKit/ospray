// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "Material.ih"
#include "texture/TextureParam.ih"
#include "render/pathtracer/bsdfs/MultiBSDF.ih"
#include "render/pathtracer/bsdfs/Transmission.ih"
#include "render/pathtracer/bsdfs/Lambert.ih"
#include "render/pathtracer/bsdfs/Specular.ih"

struct OBJ
{
  PathTraceMaterial super;

  TextureParam map_d;
  float d; // cut-out opacity
  TextureParam map_Kd;
  vec3f Kd;
  TextureParam map_Ks;
  vec3f Ks;
  TextureParam map_Ns;
  float Ns;
  vec3f Tf; // transmission filter
  TextureParam map_Bump;
  linear2f rot_Bump; // just the inverse of rotational/mirror part (must be orthonormal) of tc xfrom
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform OBJ_getBSDF(const uniform PathTraceMaterial* uniform super,
                                        uniform ShadingContext* uniform ctx,
                                        const DifferentialGeometry& dg,
                                        const Ray& ray,
                                        const Medium& currentMedium)
{
  uniform const OBJ* uniform self = (uniform const OBJ* uniform)super;
  varying BSDF* uniform bsdf = MultiBSDF_create(ctx);
  // textures modify (mul) values, see http://paulbourke.net/dataformats/mtl/

  /*! normal map */
  vec3f shadingNormal;
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
  } else {
    shadingNormal = dg.Ns;
  }

  varying linear3f* uniform shadingFrame = LinearSpace3f_create(ctx, frame(shadingNormal));

  /*! cut-out opacity */
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
    MultiBSDF_add(bsdf, Lambert_create(ctx, shadingFrame, Kd), luminance(Kd));

  /*! transmission component */
  vec3f T = self->Tf * d + make_vec3f(1.f - d);
  if (reduce_max(T) > 0.0f)
    MultiBSDF_add(bsdf, Transmission_create(ctx, shadingFrame, T), luminance(T));

  /*! specular component */
  float Ns = self->Ns * get1f(self->map_Ns, dg.st, 1.0f);
  vec3f Ks = d * self->Ks * get3f(self->map_Ks, dg.st, make_vec3f(1.f));
  if (ne(Ks,make_vec3f(0.0f)))
    MultiBSDF_add(bsdf, Specular_create(ctx, shadingFrame, Ks, Ns), luminance(Ks));

  return bsdf;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void PathTracer_OBJ_set(
    void* uniform _self,
    void* uniform map_d,  const uniform affine2f &xform_d,  const uniform float d,
    void* uniform map_Kd, const uniform affine2f &xform_Kd, const uniform vec3f &Kd,
    void* uniform map_Ks, const uniform affine2f &xform_Ks, const uniform vec3f &Ks,
    void* uniform map_Ns, const uniform affine2f &xform_Ns, const uniform float Ns, const uniform vec3f &Tf,
    void* uniform map_Bump, const uniform affine2f &xform_Bump, const uniform linear2f &rot_Bump)
{
  uniform OBJ* uniform self = (uniform OBJ* uniform)_self;
  self->map_d  = make_TextureParam((uniform Texture2D*)map_d,  xform_d);  self->d  = d;
  self->map_Kd = make_TextureParam((uniform Texture2D*)map_Kd, xform_Kd); self->Kd = Kd;
  self->map_Ks = make_TextureParam((uniform Texture2D*)map_Ks, xform_Ks); self->Ks = Ks;
  self->map_Ns = make_TextureParam((uniform Texture2D*)map_Ns, xform_Ns); self->Ns = Ns; self->Tf = Tf;
  self->map_Bump = make_TextureParam((uniform Texture2D*)map_Bump, xform_Bump); self->rot_Bump = rot_Bump;
}

vec3f OBJ_getTransparency(const uniform PathTraceMaterial* uniform super,
                          const DifferentialGeometry& dg,
                          const Ray& ray,
                          const Medium& currentMedium)
{
  uniform const OBJ* uniform self = (uniform const OBJ* uniform)super;

  /*! cut-out opacity */
  float d = self->d * get1f(self->map_d, dg.st, 1.f);

  /*! transmission component */
  vec3f T = self->Tf * d + make_vec3f(1.f - d);

  return T;
}

void OBJ_Constructor(uniform OBJ* uniform self)
{
  PathTraceMaterial_Constructor(&self->super, OBJ_getBSDF, OBJ_getTransparency, NULL, NULL);

  uniform affine2f xform = make_AffineSpace2f_identity();

  PathTracer_OBJ_set(self,
                     NULL, xform, 1.0f,
                     NULL, xform, make_vec3f(0.8f),
                     NULL, xform, make_vec3f(0.f),
                     NULL, xform, 2.0f, make_vec3f(0.f),
                     NULL, xform, make_LinearSpace2f_identity());
}

export void* uniform PathTracer_OBJ_create()
{
  uniform OBJ* uniform self = uniform new uniform OBJ;
  OBJ_Constructor(self);
  return self;
}
