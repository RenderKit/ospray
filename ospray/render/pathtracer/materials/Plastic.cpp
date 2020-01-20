// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Plastic_ispc.h"
#include "common/Material.h"

namespace ospray {
namespace pathtracer {

struct Plastic : public ospray::Material
{
  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::Plastic";
  }

  Plastic()
  {
    ispcEquivalent = ispc::PathTracer_Plastic_create();
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    const vec3f pigmentColor = getParam<vec3f>("pigmentColor", vec3f(1.f));
    const float eta = getParam<float>("eta", 1.4f);
    const float roughness = getParam<float>("roughness", 0.01f);

    ispc::PathTracer_Plastic_set(
        getIE(), (const ispc::vec3f &)pigmentColor, eta, roughness);
  }
};

OSP_REGISTER_MATERIAL(pt, Plastic, plastic);
} // namespace pathtracer
} // namespace ospray
