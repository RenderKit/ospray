// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Metal.h"
#include "common/Data.h"
// ispc
#include "render/materials/Metal_ispc.h"

namespace ospray {
namespace pathtracer {

Metal::Metal()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_METAL;
  getSh()->super.getBSDF = ispc::Metal_getBSDF_addr();
}

std::string Metal::toString() const
{
  return "ospray::pathtracer::Metal";
}

void Metal::commit()
{
  auto ior = getParamDataT<vec3f>("ior");
  const vec3f &eta = getParam<vec3f>("eta", vec3f(RGB_AL_ETA));
  const vec3f &k = getParam<vec3f>("k", vec3f(RGB_AL_K));
  MaterialParam1f roughness = getMaterialParam1f("roughness", .1f);

  if (ior) {
    // resample, relies on ordered samples
    spectrum etaResampled;
    spectrum kResampled;
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
    getSh()->spectral = true;
    getSh()->eta = etaResampled;
    getSh()->k = kResampled;
  } else {
    // default to Aluminium, used when ior not given
    getSh()->spectral = false;
    getSh()->etaRGB = eta;
    getSh()->kRGB = k;
  }

  getSh()->roughness = roughness.factor;
  getSh()->roughnessMap = roughness.tex;
}

} // namespace pathtracer
} // namespace ospray
