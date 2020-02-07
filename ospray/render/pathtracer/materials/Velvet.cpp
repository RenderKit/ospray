// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Velvet_ispc.h"
#include "common/Material.h"

namespace ospray {
namespace pathtracer {

struct Velvet : public ospray::Material
{
  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::Velvet";
  }

  Velvet()
  {
    ispcEquivalent = ispc::PathTracer_Velvet_create();
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    vec3f reflectance = getParam<vec3f>("reflectance", vec3f(.4f, 0.f, 0.f));
    float backScattering = getParam<float>("backScattering", .5f);
    vec3f horizonScatteringColor =
        getParam<vec3f>("horizonScatteringColor", vec3f(.75f, .1f, .1f));
    float horizonScatteringFallOff =
        getParam<float>("horizonScatteringFallOff", 10);

    ispc::PathTracer_Velvet_set(getIE(),
        (const ispc::vec3f &)reflectance,
        (const ispc::vec3f &)horizonScatteringColor,
        horizonScatteringFallOff,
        backScattering);
  }
};

OSP_REGISTER_MATERIAL(pathtracer, Velvet, velvet);
} // namespace pathtracer
} // namespace ospray
