// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "../bsdfs/Lambert.ih"
#include "../bsdfs/Conductor.ih"
#include "../bsdfs/MicrofacetConductor.ih"
#include "../bsdfs/DielectricLayer.ih"
#include "../bsdfs/MicrofacetDielectricLayer.ih"
#include "math/spectrum.ih"

struct Principled
{
  PathTraceMaterial super;

  vec3f baseColor;
  TextureParam map_baseColor;
  
  float metallic;
  TextureParam map_metallic;
  
  float specular; // dielectric specular reflectivity in [0, 1]; specular = sqr((ior-1)/(ior+1))/0.08
  TextureParam map_specular;
  
  vec3f edgeColor; // metallic reflectivity at grazing angle (90 deg) / edge tint
  TextureParam map_edgeColor;
  
  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam map_roughness;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform Principled_getBSDF(const uniform PathTraceMaterial* uniform super,
                                               uniform ShadingContext* uniform ctx,
                                               const DifferentialGeometry& dg,
                                               const Ray& ray,
                                               const Medium& currentMedium)
{
  const uniform Principled* uniform self = (const uniform Principled* uniform)super;
  varying BSDF* uniform bsdf = MultiBSDF_create(ctx);
  varying linear3f* uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));

  const vec3f baseColor = self->baseColor * get3f(self->map_baseColor, dg.st, make_vec3f(1.f)) * make_vec3f(dg.color);
  const float metallic = self->metallic * get1f(self->map_metallic, dg.st, 1.f);
  const float roughness = self->roughness * get1f(self->map_roughness, dg.st, 1.f);

  // dielectric diffuse+specular
  const float diffuse = 1.f - metallic;
  if (diffuse > 1e-5f) {
    // diffuse
    varying BSDF* uniform diffuseBsdf = Lambert_create(ctx, frame, baseColor);
    
    // specular
    const float specular = self->specular * get1f(self->map_specular, dg.st, 1.f);
    if (specular > 1e-5f) {
      const float specularIor = (2.f / (1.f - sqrt(0.08f * specular))) - 1.f;
      if (roughness < 1e-5f)
        diffuseBsdf = DielectricLayer_create(ctx, frame, diffuseBsdf, rcp(specularIor), make_vec3f(1.f), 1.f);
      else
        diffuseBsdf = MicrofacetDielectricLayer_create(ctx, frame, diffuseBsdf, rcp(specularIor), make_vec3f(1.f), 1.f, roughness);
    }

    MultiBSDF_add(bsdf, diffuseBsdf, diffuse, luminance(baseColor)*diffuse);
  }
    
  // conductor
  if (metallic > 1e-5f) {
    const vec3f edgeColor = self->edgeColor * get3f(self->map_edgeColor, dg.st, make_vec3f(1.f));
    Fresnel* uniform fresnel = FresnelConductorArtistic_create(ctx, baseColor, edgeColor);
    varying BSDF* uniform conductorBsdf;
    if (roughness < 1e-5f)
      conductorBsdf = Conductor_create(ctx, frame, fresnel);
    else
      conductorBsdf = MicrofacetConductor_create(ctx, frame, fresnel, roughness);
    MultiBSDF_add(bsdf, conductorBsdf, metallic, luminance(baseColor)*metallic);
  }

  return bsdf;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Principled_set(void* uniform _self,
    const uniform vec3f& baseColor, void* uniform map_baseColor, const uniform affine2f& xform_baseColor,
    uniform float metallic, void* uniform map_metallic, const uniform affine2f& xform_metallic,
    uniform float specular, void* uniform map_specular, const uniform affine2f& xform_specular,
    const uniform vec3f& edgeColor, void* uniform map_edgeColor, const uniform affine2f& xform_edgeColor,
    uniform float roughness, void* uniform map_roughness, const uniform affine2f& xform_roughness)
{
  Principled* uniform self = (Principled* uniform)_self;

  self->baseColor = baseColor;
  self->map_baseColor = make_TextureParam((uniform Texture2D*)map_baseColor, xform_baseColor);
  
  self->metallic = metallic;
  self->map_metallic = make_TextureParam((uniform Texture2D*)map_metallic, xform_metallic);
  
  self->specular = specular;
  self->map_specular = make_TextureParam((uniform Texture2D*)map_specular, xform_specular);

  self->edgeColor = edgeColor;
  self->map_edgeColor = make_TextureParam((uniform Texture2D*)map_edgeColor, xform_edgeColor);

  self->roughness = roughness;
  self->map_roughness = make_TextureParam((uniform Texture2D*)map_roughness, xform_roughness);
}

export void* uniform PathTracer_Principled_create()
{
  Principled* uniform self = uniform new Principled;
  PathTraceMaterial_Constructor(&self->super, Principled_getBSDF);
  uniform affine2f xform = make_AffineSpace2f_identity();
  PathTracer_Principled_set(self,
    make_vec3f(0.9f), NULL, xform,
    0.0f, NULL, xform,
    0.0f, NULL, xform,
    make_vec3f(1.f), NULL, xform,
    0.1f, NULL, xform);
  return self;
}
