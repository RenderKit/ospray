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

#include "arcball.h"

using namespace ospcommon;

Arcball::Arcball(const box3f &worldBounds)
  : translation(one), rotation(one)
{
  vec3f diag = worldBounds.size();
  zoomSpeed = max(length(diag) / 150.0, 0.001);
  diag = max(diag, vec3f(0.3f * length(diag)));

  lookAt = AffineSpace3f::lookat(vec3f(0, 0, 1), vec3f(0, 0, 0), vec3f(0, 1, 0));
  translation = AffineSpace3f::translate(vec3f(0, 0, diag.z));
  updateCamera();
}
void Arcball::rotate(const vec2f &from, const vec2f &to) {
  rotation = screenToArcball(to) * screenToArcball(from) * rotation;
  updateCamera();
}
void Arcball::zoom(float amount) {
  amount *= zoomSpeed;
  translation = AffineSpace3f::translate(vec3f(0, 0, amount)) * translation;
  updateCamera();
}
vec3f Arcball::eyePos() const {
  return xfmPoint(inv_camera, vec3f(0, 0, 1));
}
vec3f Arcball::lookDir() const {
  return xfmVector(inv_camera, vec3f(0, 0, 1));
}
vec3f Arcball::upDir() const {
  return xfmVector(inv_camera, vec3f(0, 1, 0));
}
void Arcball::updateCamera() {
  const AffineSpace3f rot = LinearSpace3f(rotation);
  const AffineSpace3f camera = translation * lookAt * rot;
  inv_camera = rcp(camera);
}
Quaternion3f Arcball::screenToArcball(const vec2f &p) {
  const float dist = dot(p, p);
  // If we're on/in the sphere return the point on it
  if (dist <= 1.f){
    return Quaternion3f(0, p.x, p.y, std::sqrt(1.f - dist));
  } else {
    // otherwise we project the point onto the sphere
    const vec2f unitDir = normalize(p);
    return Quaternion3f(0, unitDir.x, unitDir.y, 0);
  }
}


