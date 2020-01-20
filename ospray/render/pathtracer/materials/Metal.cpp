// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Metal_ispc.h"
#include "common/Data.h"
#include "common/Material.h"
#include "math/spectrum.h"
#include "texture/Texture2D.h"

namespace ospray {
namespace pathtracer {

struct Metal : public ospray::Material
{
  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::Metal";
  }

  Metal()
  {
    ispcEquivalent = ispc::PathTracer_Metal_create();
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    auto ior = getParamDataT<vec3f>("ior");
    float etaResampled[SPECTRUM_SAMPLES];
    float kResampled[SPECTRUM_SAMPLES];
    float *etaSpectral = nullptr;
    float *kSpectral = nullptr;
    if (ior) {
      // resample, relies on ordered samples
      auto iorP = ior->begin();
      auto iorPrev = *iorP;
      const auto iorLast = ior->end();
      float wl = SPECTRUM_FIRSTWL;
      for (int l = 0; l < SPECTRUM_SAMPLES; wl += SPECTRUM_SPACING, l++) {
        for (; iorP != iorLast && iorP->x < wl; iorP++)
          iorPrev = *iorP;
        if (iorP->x == iorPrev.x) {
          etaResampled[l] = iorPrev.y;
          kResampled[l] = iorPrev.z;
        } else {
          auto f = (wl - iorPrev.x) / (iorP->x - iorPrev.x);
          etaResampled[l] = (1.f - f) * iorPrev.y + f * iorP->y;
          kResampled[l] = (1.f - f) * iorPrev.z + f * iorP->z;
        }
      }
      etaSpectral = etaResampled;
      kSpectral = kResampled;
    }

    // default to Aluminium, used when ior not given
    const vec3f &eta = getParam<vec3f>("eta", vec3f(RGB_AL_ETA));
    const vec3f &k = getParam<vec3f>("k", vec3f(RGB_AL_K));

    const float roughness = getParam<float>("roughness", 0.1f);
    Texture2D *map_roughness = (Texture2D *)getParamObject("map_roughness");
    affine2f xform_roughness = getTextureTransform("map_roughness");

    ispc::PathTracer_Metal_set(getIE(),
        etaSpectral,
        kSpectral,
        (const ispc::vec3f &)eta,
        (const ispc::vec3f &)k,
        roughness,
        map_roughness ? map_roughness->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_roughness);
  }
};

OSP_REGISTER_MATERIAL(pathtracer, Metal, metal);
} // namespace pathtracer
} // namespace ospray
