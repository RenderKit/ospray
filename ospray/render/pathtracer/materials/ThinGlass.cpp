// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ThinGlass_ispc.h"
#include "common/Material.h"
#include "texture/Texture2D.h"

namespace ospray {
namespace pathtracer {

struct ThinGlass : public ospray::Material
{
  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::ThinGlass";
  }

  ThinGlass()
  {
    ispcEquivalent = ispc::PathTracer_ThinGlass_create();
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    const float eta = getParam<float>("eta", 1.5f);
    const vec3f &attenuationColor =
        getParam<vec3f>("attenuationColor", vec3f(1.f));
    const float attenuationDistance =
        getParam<float>("attenuationDistance", 1.f);
    const float thickness = getParam<float>("thickness", 1.f);

    Texture2D *map_attenuationColor =
        (Texture2D *)getParamObject("map_attenuationColor");
    affine2f xform_attenuationColor =
        getTextureTransform("map_attenuationColor");

    ispc::PathTracer_ThinGlass_set(getIE(),
        eta,
        (const ispc::vec3f &)attenuationColor,
        map_attenuationColor ? map_attenuationColor->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_attenuationColor,
        attenuationDistance,
        thickness);
  }
};

OSP_REGISTER_MATERIAL(pathtracer, ThinGlass, thinGlass);
} // namespace pathtracer
} // namespace ospray
