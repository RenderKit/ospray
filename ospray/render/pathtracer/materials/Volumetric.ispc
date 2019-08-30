// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "render/pathtracer/bsdfs/HeyneyGreenstein.ih"

struct VolumetricMaterial
{
  PathTraceMaterial super;

  vec3f albedo;
  float meanCosine;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* varying Volumetric_getBSDF(const uniform PathTraceMaterial* uniform super,
                                        uniform ShadingContext* uniform ctx,
                                        const DifferentialGeometry& dg,
                                        const Ray& ray,
                                        const Medium& currentMedium)
{
  uniform const VolumetricMaterial* uniform self = (uniform const VolumetricMaterial* uniform)super;

  varying linear3f* uniform shadingFrame =
    LinearSpace3f_create(ctx, frame(ray.dir));

  varying BSDF* uniform bsdf = HeyneyGreenstein_create(ctx, shadingFrame, self->meanCosine);
  return bsdf;
}

vec3f Volumetric_getTransparency(const uniform PathTraceMaterial* uniform super,
                          const DifferentialGeometry& dg,
                          const Ray& ray,
                          const Medium& currentMedium)
{
  return make_vec3f(1.f);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void PathTracer_Volumetric_set(
    void* uniform _self, const uniform vec3f &albedo, const uniform float meanCosine)
{
  uniform VolumetricMaterial* uniform self = (uniform VolumetricMaterial* uniform)_self;
  self->albedo = albedo;
  self->meanCosine = meanCosine;
}

void Volumetric_Constructor(uniform VolumetricMaterial* uniform self)
{
  PathTraceMaterial_Constructor(&self->super, Volumetric_getBSDF, Volumetric_getTransparency);

  uniform affine2f xform = make_AffineSpace2f_identity();

  PathTracer_Volumetric_set(self, make_vec3f(0.8f), 0.f);
}

export void* uniform PathTracer_Volumetric_create()
{
  uniform VolumetricMaterial* uniform self = uniform new uniform VolumetricMaterial;
  Volumetric_Constructor(self);
  return self;
}
