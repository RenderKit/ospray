// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "HDRILight.h"
#include "HDRILight_ispc.h"

namespace ospray {

  HDRILight::HDRILight()
  {
    ispcEquivalent = ispc::HDRILight_create();
  }

  HDRILight::~HDRILight()
  {
    ispc::HDRILight_destroy(getIE());
    ispcEquivalent = nullptr;
  }

  std::string HDRILight::toString() const
  {
    return "ospray::HDRILight";
  }

  void HDRILight::commit()
  {
    Light::commit();
    up = getParam3f("up", vec3f(0.f, 1.f, 0.f));
    dir = getParam3f("dir", vec3f(0.f, 0.f, 1.f));
    intensity = getParam1f("intensity", 1.f);
    map  = (Texture2D*)getParamObject("map", nullptr);

    linear3f frame;
    frame.vx = normalize(-dir);
    frame.vy = normalize(cross(frame.vx, up));
    frame.vz = cross(frame.vx, frame.vy);

    ispc::HDRILight_set(getIE(),
                        (const ispc::LinearSpace3f&)frame,
                        map ? map->getIE() : nullptr,
                        intensity);
  }

  OSP_REGISTER_LIGHT(HDRILight, hdri);

} // ::ospray
