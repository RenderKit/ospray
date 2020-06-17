// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.ih"
#include "render/pathtracer/bsdfs/Lambert.ih"
#include "render/pathtracer/bsdfs/MultiBSDF.ih"
#include "render/pathtracer/bsdfs/Specular.ih"
#include "render/pathtracer/bsdfs/Transmission.ih"

struct OBJ
{
  PathTraceMaterial super;

  TextureParam dMap;
  float d; // cut-out opacity
  TextureParam KdMap;
  vec3f Kd;
  TextureParam KsMap;
  vec3f Ks;
  TextureParam NsMap;
  float Ns;
  vec3f Tf; // transmission filter
  TextureParam bumpMap;
  linear2f bumpRot; // just the inverse of rotational/mirror part (must be
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

vec3f OBJ_getTransparency(const uniform PathTraceMaterial *uniform super,
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

export void PathTracer_OBJ_set(void *uniform _self,
    const uniform TextureParam &dMap,
    const uniform float d,
    const uniform TextureParam &KdMap,
    const uniform vec3f &Kd,
    const uniform TextureParam &KsMap,
    const uniform vec3f &Ks,
    const uniform TextureParam &NsMap,
    const uniform float Ns,
    const uniform vec3f &Tf,
    const uniform TextureParam &bumpMap,
    const uniform linear2f &bumpRot)
{
  uniform OBJ *uniform self = (uniform OBJ * uniform) _self;
  self->dMap = dMap;
  self->d = d;
  self->KdMap = KdMap;
  self->Kd = Kd;
  self->KsMap = KsMap;
  self->Ks = Ks;
  self->NsMap = NsMap;
  self->Ns = Ns;
  self->Tf = Tf;
  self->bumpMap = bumpMap;
  self->bumpRot = bumpRot;
}

void OBJ_Constructor(uniform OBJ *uniform self)
{
  PathTraceMaterial_Constructor(&self->super, OBJ_getBSDF, OBJ_getTransparency);

  uniform TextureParam tex = make_TextureParam();
  PathTracer_OBJ_set(self,
      tex,
      1.0f,
      tex,
      make_vec3f(0.8f),
      tex,
      make_vec3f(0.f),
      tex,
      2.0f,
      make_vec3f(0.f),
      tex,
      make_LinearSpace2f_identity());
}

export void *uniform PathTracer_OBJ_create()
{
  uniform OBJ *uniform self = uniform new uniform OBJ;
  OBJ_Constructor(self);
  return self;
}
