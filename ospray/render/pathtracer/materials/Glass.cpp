// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Glass_ispc.h"
#include "common/Material.h"
#include "texture/Texture2D.h"

namespace ospray {
namespace pathtracer {
struct Glass : public ospray::Material
{
  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::Glass";
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    if (getIE() == nullptr) {
      ispcEquivalent = ispc::PathTracer_Glass_create();
    }

    const float etaInside =
        getParam<float>("etaInside", getParam<float>("eta", 1.5f));

    const float etaOutside = getParam<float>("etaOutside", 1.f);

    const vec3f &attenuationColorInside =
        getParam<vec3f>("attenuationColorInside",
            getParam<vec3f>("attenuationColor", vec3f(1.f)));

    const vec3f &attenuationColorOutside =
        getParam<vec3f>("attenuationColorOutside", vec3f(1.f));

    const float attenuationDistance =
        getParam<float>("attenuationDistance", 1.0f);

    ispc::PathTracer_Glass_set(ispcEquivalent,
        etaInside,
        (const ispc::vec3f &)attenuationColorInside,
        etaOutside,
        (const ispc::vec3f &)attenuationColorOutside,
        attenuationDistance);
  }
};

OSP_REGISTER_MATERIAL(pathtracer, Glass, glass);
} // namespace pathtracer
} // namespace ospray
