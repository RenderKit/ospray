// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Luminous_ispc.h"
#include "common/Material.h"

namespace ospray {
namespace pathtracer {

struct Luminous : public ospray::Material
{
  Luminous()
  {
    ispcEquivalent = ispc::PathTracer_Luminous_create();
  }

  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::Luminous";
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    const vec3f radiance = getParam<vec3f>("color", vec3f(1.f))
        * getParam<float>("intensity", 1.f);
    const float transparency = getParam<float>("transparency", 0.f);

    ispc::PathTracer_Luminous_set(
        getIE(), (const ispc::vec3f &)radiance, transparency);
  }
};

OSP_REGISTER_MATERIAL(pathtracer, Luminous, luminous);
} // namespace pathtracer
} // namespace ospray
