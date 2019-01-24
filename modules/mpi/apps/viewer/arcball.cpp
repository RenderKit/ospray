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

#include "arcball.h"

using namespace ospcommon;

Arcball::Arcball(const box3f &worldBounds, const vec2i &screenDims)
  : zoomSpeed(1),
  invScreen(vec2f(1.0) / vec2f(screenDims)),
  centerTranslation(one),
  translation(one),
  rotation(one)
{
  vec3f diag = worldBounds.size();
  zoomSpeed = max(length(diag) / 150.0, 0.001);
  diag = max(diag, vec3f(0.3 * length(diag)));

  centerTranslation = AffineSpace3f::translate(-worldBounds.center());
  translation = AffineSpace3f::translate(vec3f(0, 0, 0.9 * length(diag)));
  rotate(vec2f(0.5, 0.5), vec2f(0.7, 0.4));
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
void Arcball::pan(const ospcommon::vec2f &delta) {
  const vec3f t = vec3f(delta.x * invScreen.x, delta.y * invScreen.y, 0);
  const vec3f worldt = translation.p.z * xfmVector(invCamera, t);
  centerTranslation = AffineSpace3f::translate(worldt) * centerTranslation;
  updateCamera();
}
vec3f Arcball::eyePos() const {
  return xfmPoint(invCamera, vec3f(0, 0, 1));
}
vec3f Arcball::center() const {
  return -centerTranslation.p;
}
vec3f Arcball::lookDir() const {
  return xfmVector(invCamera, vec3f(0, 0, 1));
}
vec3f Arcball::upDir() const {
  return xfmVector(invCamera, vec3f(0, 1, 0));
}
void Arcball::updateCamera() {
  const AffineSpace3f rot = LinearSpace3f(rotation);
  const AffineSpace3f camera = translation * rot * centerTranslation;
  invCamera = rcp(camera);
}
void Arcball::updateScreen(const vec2i &screenDims) {
  invScreen = vec2f(1) / vec2f(screenDims);
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


