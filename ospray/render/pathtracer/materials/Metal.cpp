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

#include "common/Material.h"
#include "common/Data.h"
#include "texture/Texture2D.h"
#include "math/spectrum.h"
#include "Metal_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct Metal : public ospray::Material
    {
      //! \brief common function to help printf-debugging
      /*! Every derived class should overrride this! */
      virtual std::string toString() const override
      { return "ospray::pathtracer::Metal"; }

      Metal()
      {
        ispcEquivalent = ispc::PathTracer_Metal_create();
      }

      //! \brief commit the material's parameters
      virtual void commit() override
      {
        auto ior = getParamData("ior");
        // default to Aluminium
        float etaResampled[SPECTRUM_SAMPLES] = SPECTRUM_AL_ETA;
        float kResampled[SPECTRUM_SAMPLES] = SPECTRUM_AL_K;
        if (ior && ior->data && ior->size() > 0) {
          if (ior->type != OSP_FLOAT3)
            throw std::runtime_error("Metal::ior must have data type OSP_FLOAT3 (wavelength, eta, k)[]");
          // resample, relies on ordered samples
          auto iorP = (vec3f*)ior->data;
          auto iorPrev = *iorP;
          const auto iorLast = (vec3f*)ior->data + ior->size()-1;
          float wl = SPECTRUM_FIRSTWL;
          for(int l = 0; l < SPECTRUM_SAMPLES; wl += SPECTRUM_SPACING, l++) {
            for(; iorP != iorLast && iorP->x < wl; iorP++)
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
        }

        const float roughness = getParamf("roughness", 0.1f);
        Texture2D *map_roughness = (Texture2D*)getParamObject("map_roughness");
        affine2f xform_roughness = getTextureTransform("map_roughness"); 

        ispc::PathTracer_Metal_set(getIE()
            , etaResampled
            , kResampled
            , roughness
            , map_roughness ? map_roughness->getIE() : nullptr
            , (const ispc::AffineSpace2f&)xform_roughness
            );
      }
    };

    OSP_REGISTER_MATERIAL(Metal,PathTracer_Metal);
  }
}
