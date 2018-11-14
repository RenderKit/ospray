// ======================================================================== //
// Copyright 2017-2018 Intel Corporation                                    //
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

#include <ospcommon/AffineSpace.h>

struct Arcball {
  Arcball(const ospcommon::box3f &worldBounds,
          const ospcommon::vec2i &screenDims);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(const ospcommon::vec2f &from, const ospcommon::vec2f &to);
  void zoom(float amount);
  void pan(const ospcommon::vec2f &delta);
  ospcommon::vec3f eyePos() const;
  ospcommon::vec3f center() const;
  ospcommon::vec3f lookDir() const;
  ospcommon::vec3f upDir() const;
  void updateScreen(const ospcommon::vec2i &screenDims);

private:
  void updateCamera();
  // Project the point in [-1, 1] screen space onto the arcball sphere
  ospcommon::Quaternion3f screenToArcball(const ospcommon::vec2f &p);

  float zoomSpeed;
  ospcommon::vec2f invScreen;
  ospcommon::AffineSpace3f centerTranslation, translation, invCamera;
  ospcommon::Quaternion3f rotation;
};

