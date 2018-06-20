// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

struct Mix
{
  PathTraceMaterial super;

  float factor;
  TextureParam map_factor;
  PathTraceMaterial *mat1;
  PathTraceMaterial *mat2;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* varying Mix_getBSDF(const uniform PathTraceMaterial* uniform super,
                                        uniform ShadingContext* uniform ctx,
                                        const DifferentialGeometry& dg,
                                        const Ray& ray,
                                        const Medium& currentMedium)
{
  const Mix* uniform self = (const Mix* uniform)super;
  varying BSDF* uniform bsdf = MultiBSDF_create(ctx);

  float factor = self->factor * clamp(get1f(self->map_factor, dg, 1.f));

  if (self->mat1)
    MultiBSDF_add(bsdf, self->mat1->getBSDF(self->mat1, ctx, dg, ray, currentMedium), 1.0f - factor, 1.0f - factor);

  if (self->mat2)
    MultiBSDF_add(bsdf, self->mat2->getBSDF(self->mat2, ctx, dg, ray, currentMedium), factor, factor);

  return bsdf;
}

vec3f Mix_getTransparency(const uniform PathTraceMaterial* uniform super,
                          const DifferentialGeometry& dg,
                          const Ray& ray,
                          const Medium& currentMedium)
{
  const Mix* uniform self = (const Mix* uniform)super;

  vec3f t1 = make_vec3f(0.f);
  if (self->mat1)
    t1 = self->mat1->getTransparency(self->mat1, dg, ray, currentMedium);

  vec3f t2 = make_vec3f(0.f);
  if (self->mat2)
    t2 = self->mat2->getTransparency(self->mat2, dg, ray, currentMedium);

  float factor = self->factor * clamp(get1f(self->map_factor, dg, 1.f));
  return lerp(factor, t1, t2);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void PathTracer_Mix_set(void* uniform _self
    , uniform float factor
    , void* uniform map_factor
    , const uniform affine2f &xform_factor
    , void* uniform _mat1
    , void* uniform _mat2
    )
{
  Mix* uniform self = (Mix* uniform)_self;
  self->factor = clamp(factor);
  self->map_factor = make_TextureParam((uniform Texture2D*)map_factor, xform_factor);
  self->mat1 = (PathTraceMaterial* uniform)_mat1;
  self->mat2 = (PathTraceMaterial* uniform)_mat2;

  uniform vec3f emission = make_vec3f(0.f);
  if (self->mat1)
    emission = (1.f - self->factor) * self->mat1->emission;
  if (self->mat2)
    emission = emission + self->factor * self->mat2->emission;
  self->super.emission = emission;
}

void Mix_Constructor(uniform Mix* uniform self)
{
  PathTraceMaterial_Constructor(&self->super, Mix_getBSDF, Mix_getTransparency);
  PathTracer_Mix_set(self, 0.5f, NULL, make_AffineSpace2f_identity(), NULL, NULL);
}

export void* uniform PathTracer_Mix_create()
{
  Mix* uniform self = uniform new uniform Mix;
  Mix_Constructor(self);
  return self;
}
