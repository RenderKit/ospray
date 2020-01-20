// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.ih"
#include "render/pathtracer/bsdfs/Lambert.ih"
#include "render/pathtracer/bsdfs/MultiBSDF.ih"
#include "render/pathtracer/bsdfs/Specular.ih"
#include "render/pathtracer/bsdfs/Transmission.ih"

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
  linear2f rot_Bump; // just the inverse of rotational/mirror part (must be
                     // orthonormal) of tc xfrom
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying OBJ_getBSDF(
    const uniform PathTraceMaterial *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  uniform const OBJ *uniform self = (uniform const OBJ *uniform)super;
  varying BSDF *uniform bsdf = MultiBSDF_create(ctx);
  // textures modify (mul) values, see http://paulbourke.net/dataformats/mtl/

  /*! normal map */
  varying linear3f *uniform shadingFrame = LinearSpace3f_create(
      ctx, makeShadingFrame(dg, self->map_Bump, self->rot_Bump));

  /*! cut-out opacity */
  float d = self->d * get1f(self->map_d, dg, 1.f) * dg.color.w;

  /*! diffuse component */
  vec3f Kd = self->Kd;
  if (valid(self->map_Kd)) {
    vec4f Kd_from_map = get4f(self->map_Kd, dg);
    Kd = Kd * make_vec3f(Kd_from_map);
    d *= Kd_from_map.w;
  }
  Kd = Kd * d * make_vec3f(dg.color);
  if (reduce_max(Kd) > 0.0f)
    MultiBSDF_add(
        bsdf, Lambert_create(ctx, shadingFrame, Kd), 1.f, luminance(Kd));

  /*! transmission component */
  vec3f T = self->Tf * d + make_vec3f(1.f - d);
  if (reduce_max(T) > 0.0f)
    MultiBSDF_add(
        bsdf, Transmission_create(ctx, shadingFrame, T), 1.f, luminance(T));

  /*! specular component */
  float Ns = self->Ns * get1f(self->map_Ns, dg, 1.0f);
  vec3f Ks = d * self->Ks * get3f(self->map_Ks, dg, make_vec3f(1.f));
  if (reduce_max(Ks) > 0.0f)
    MultiBSDF_add(
        bsdf, Specular_create(ctx, shadingFrame, Ks, Ns), 1.f, luminance(Ks));

  return bsdf;
}

vec3f OBJ_getTransparency(const uniform PathTraceMaterial *uniform super,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  uniform const OBJ *uniform self = (uniform const OBJ *uniform)super;

  /*! cut-out opacity */
  float d = self->d * get1f(self->map_d, dg, 1.f) * dg.color.w;
  if (hasAlpha(self->map_Kd)) {
    vec4f Kd_from_map = get4f(self->map_Kd, dg);
    d *= Kd_from_map.w;
  }

  /*! transmission component */
  vec3f T = self->Tf * d + make_vec3f(1.f - d);

  return T;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void PathTracer_OBJ_set(void *uniform _self,
    void *uniform map_d,
    const uniform affine2f &xform_d,
    const uniform float d,
    void *uniform map_Kd,
    const uniform affine2f &xform_Kd,
    const uniform vec3f &Kd,
    void *uniform map_Ks,
    const uniform affine2f &xform_Ks,
    const uniform vec3f &Ks,
    void *uniform map_Ns,
    const uniform affine2f &xform_Ns,
    const uniform float Ns,
    const uniform vec3f &Tf,
    void *uniform map_Bump,
    const uniform affine2f &xform_Bump,
    const uniform linear2f &rot_Bump)
{
  uniform OBJ *uniform self = (uniform OBJ * uniform) _self;
  self->map_d = make_TextureParam((uniform Texture2D *)map_d, xform_d);
  self->d = d;
  self->map_Kd = make_TextureParam((uniform Texture2D *)map_Kd, xform_Kd);
  self->Kd = Kd;
  self->map_Ks = make_TextureParam((uniform Texture2D *)map_Ks, xform_Ks);
  self->Ks = Ks;
  self->map_Ns = make_TextureParam((uniform Texture2D *)map_Ns, xform_Ns);
  self->Ns = Ns;
  self->Tf = Tf;
  self->map_Bump = make_TextureParam((uniform Texture2D *)map_Bump, xform_Bump);
  self->rot_Bump = rot_Bump;
}

void OBJ_Constructor(uniform OBJ *uniform self)
{
  PathTraceMaterial_Constructor(&self->super, OBJ_getBSDF, OBJ_getTransparency);

  uniform affine2f xform = make_AffineSpace2f_identity();

  PathTracer_OBJ_set(self,
      NULL,
      xform,
      1.0f,
      NULL,
      xform,
      make_vec3f(0.8f),
      NULL,
      xform,
      make_vec3f(0.f),
      NULL,
      xform,
      2.0f,
      make_vec3f(0.f),
      NULL,
      xform,
      make_LinearSpace2f_identity());
}

export void *uniform PathTracer_OBJ_create()
{
  uniform OBJ *uniform self = uniform new uniform OBJ;
  OBJ_Constructor(self);
  return self;
}
