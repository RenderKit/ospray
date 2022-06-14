// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OBJ.ih"
#include "render/Material.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "render/bsdfs/Specular.ih"
#include "render/bsdfs/Transmission.ih"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying OBJ_getBSDF(const uniform Material *uniform super,
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
      ctx, makeShadingFrame(dg, self->bumpMap, self->bumpRot));

  /*! cut-out opacity */
  float d = self->d * get1f(self->dMap, dg, 1.f) * dg.color.w;

  /*! diffuse component */
  vec3f Kd = self->Kd;
  if (valid(self->KdMap)) {
    vec4f Kd_from_map = get4f(self->KdMap, dg);
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
  float Ns = self->Ns * get1f(self->NsMap, dg, 1.0f);
  vec3f Ks = d * self->Ks * get3f(self->KsMap, dg, make_vec3f(1.f));
  if (reduce_max(Ks) > 0.0f)
    MultiBSDF_add(
        bsdf, Specular_create(ctx, shadingFrame, Ks, Ns), 1.f, luminance(Ks));

  return bsdf;
}

vec3f OBJ_getTransparency(const uniform Material *uniform super,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  uniform const OBJ *uniform self = (uniform const OBJ *uniform)super;

  /*! cut-out opacity */
  float d = self->d * get1f(self->dMap, dg, 1.f) * dg.color.w;
  if (hasAlpha(self->KdMap)) {
    vec4f Kd_from_map = get4f(self->KdMap, dg);
    d *= Kd_from_map.w;
  }

  /*! transmission component */
  vec3f T = self->Tf * d + make_vec3f(1.f - d);

  return T;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform OBJ_getBSDF_addr()
{
  return OBJ_getBSDF;
}

export void *uniform OBJ_getTransparency_addr()
{
  return OBJ_getTransparency;
}
