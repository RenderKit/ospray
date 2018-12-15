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
#include "../bsdfs/RobustThinDielectric.ih"

struct ThinGlass
{
  PathTraceMaterial super;

  float eta; //!< reciprocal refraction index of internal medium
             // assumed to be <=1
  vec3f attenuation; //!< negative Napierian attenuation coefficient,
                     // i.e. wrt. the natural base e
  float attenuationScale; //!< factor to scale attenuation from texture due to
                          // thickness and attenuationDistance
  TextureParam map_attenuationColor;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

static inline const vec3f getAttenuation(const ThinGlass* uniform self,
    const DifferentialGeometry& dg)
{
  vec3f attenuation = self->attenuation
    + logf(make_vec3f(dg.color)) * self->attenuationScale;

  if (valid(self->map_attenuationColor)) {
    vec3f attenuationColor = get3f(self->map_attenuationColor, dg);
    attenuation = attenuation + logf(attenuationColor) * self->attenuationScale;
  }

  return attenuation;
}

const varying BSDF* varying ThinGlass_getBSDF(const PathTraceMaterial* uniform super,
                                              ShadingContext* uniform ctx,
                                              const DifferentialGeometry& dg,
                                              const Ray& ray,
                                              const Medium& currentMedium)
{
  const ThinGlass* uniform self = (const ThinGlass* uniform)super;


  varying linear3f* uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));
  return RobustThinDielectric_create(ctx, frame, self->eta, getAttenuation(self, dg));
}

vec3f ThinGlass_getTransparency(const PathTraceMaterial* uniform material,
                                const DifferentialGeometry& dg,
                                const Ray& ray,
                                const Medium& currentMedium)
{
  const ThinGlass* uniform self = (const ThinGlass* uniform)material;

  float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.0f);
  return RobustThinDielectric_getTransparency(cosThetaO, self->eta, getAttenuation(self, dg));
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void PathTracer_ThinGlass_set(void *uniform _self
    , const uniform float ior
    , const uniform vec3f& attenuationColor
    , void* uniform map_attenuationColor
    , const uniform affine2f &xform_attenuationColor
    , const uniform float attenuationDistance
    , const uniform float thickness
    )
{
  ThinGlass* uniform self = (ThinGlass* uniform)_self;
  self->eta = rcp(ior);
  self->attenuationScale = thickness*rcp(attenuationDistance);
  self->attenuation = logf(attenuationColor) * self->attenuationScale;
  self->map_attenuationColor =
    make_TextureParam((uniform Texture2D*)map_attenuationColor,
        xform_attenuationColor);
}

export void* uniform PathTracer_ThinGlass_create()
{
  ThinGlass* uniform self = uniform new ThinGlass;
  PathTraceMaterial_Constructor(&self->super, ThinGlass_getBSDF, ThinGlass_getTransparency);
  PathTracer_ThinGlass_set(self, 1.5f, make_vec3f(1.f),
      NULL, make_AffineSpace2f_identity(), 1.f, 1.f);

  return self;
}
