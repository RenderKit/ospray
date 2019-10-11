// ======================================================================== //
// Copyright 2017-2019 Intel Corporation                                    //
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

#pragma once

#include "ospcommon/math/AffineSpace.h"

class ArcballCamera
{
 public:
  ArcballCamera(const ospcommon::math::box3f &worldBounds,
                const ospcommon::math::vec2i &windowSize);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(const ospcommon::math::vec2f &from,
              const ospcommon::math::vec2f &to);
  void zoom(float amount);
  void pan(const ospcommon::math::vec2f &delta);

  ospcommon::math::vec3f eyePos() const;
  ospcommon::math::vec3f center() const;
  ospcommon::math::vec3f lookDir() const;
  ospcommon::math::vec3f upDir() const;


  void setRotation(ospcommon::math::quaternionf);

  void updateWindowSize(const ospcommon::math::vec2i &windowSize);

 protected:
  void updateCamera();

  // Project the point in [-1, 1] screen space onto the arcball sphere
  ospcommon::math::quaternionf screenToArcball(
      const ospcommon::math::vec2f &p);

  float zoomSpeed;
  ospcommon::math::vec2f invWindowSize;
  ospcommon::math::AffineSpace3f centerTranslation, translation, invCamera;
  ospcommon::math::quaternionf rotation;
};
