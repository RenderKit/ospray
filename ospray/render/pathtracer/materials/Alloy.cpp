// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Alloy_ispc.h"
#include "common/Data.h"
#include "common/Material.h"
#include "math/spectrum.h"
#include "texture/Texture2D.h"

namespace ospray {
namespace pathtracer {

struct Alloy : public ospray::Material
{
  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::Alloy";
  }

  Alloy()
  {
    ispcEquivalent = ispc::PathTracer_Alloy_create();
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    const vec3f &color = getParam<vec3f>("color", vec3f(0.9f));
    Texture2D *map_color = (Texture2D *)getParamObject("map_color");
    affine2f xform_color = getTextureTransform("map_color");

    const vec3f &edgeColor = getParam<vec3f>("edgeColor", vec3f(1.f));
    Texture2D *map_edgeColor = (Texture2D *)getParamObject("map_edgeColor");
    affine2f xform_edgeColor = getTextureTransform("map_edgeColor");

    const float roughness = getParam<float>("roughness", 0.1f);
    Texture2D *map_roughness = (Texture2D *)getParamObject("map_roughness");
    affine2f xform_roughness = getTextureTransform("map_roughness");

    ispc::PathTracer_Alloy_set(getIE(),
        (const ispc::vec3f &)color,
        map_color ? map_color->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_color,
        (const ispc::vec3f &)edgeColor,
        map_edgeColor ? map_edgeColor->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_edgeColor,
        roughness,
        map_roughness ? map_roughness->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_roughness);
  }
};

OSP_REGISTER_MATERIAL(pathtracer, Alloy, alloy);
} // namespace pathtracer
} // namespace ospray
