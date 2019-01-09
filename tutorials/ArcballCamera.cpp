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

#include "ArcballCamera.h"

ArcballCamera::ArcballCamera(const ospcommon::box3f &worldBounds,
                             const ospcommon::vec2i &windowSize)
    : zoomSpeed(1),
      invWindowSize(ospcommon::vec2f(1.0) / ospcommon::vec2f(windowSize)),
      centerTranslation(ospcommon::one),
      translation(ospcommon::one),
      rotation(ospcommon::one)
{
  ospcommon::vec3f diag = worldBounds.size();
  zoomSpeed             = ospcommon::max(length(diag) / 150.0, 0.001);
  diag = ospcommon::max(diag, ospcommon::vec3f(0.3f * length(diag)));

  centerTranslation =
      ospcommon::AffineSpace3f::translate(-worldBounds.center());
  translation =
      ospcommon::AffineSpace3f::translate(ospcommon::vec3f(0, 0, length(diag)));
  updateCamera();
}

void ArcballCamera::rotate(const ospcommon::vec2f &from,
                           const ospcommon::vec2f &to)
{
  rotation = screenToArcball(to) * screenToArcball(from) * rotation;
  updateCamera();
}

void ArcballCamera::zoom(float amount)
{
  amount *= zoomSpeed;
  translation =
      ospcommon::AffineSpace3f::translate(ospcommon::vec3f(0, 0, amount)) *
      translation;
  updateCamera();
}

void ArcballCamera::pan(const ospcommon::vec2f &delta)
{
  const ospcommon::vec3f t = ospcommon::vec3f(
      -delta.x * invWindowSize.x, delta.y * invWindowSize.y, 0);
  const ospcommon::vec3f worldt = translation.p.z * xfmVector(invCamera, t);
  centerTranslation =
      ospcommon::AffineSpace3f::translate(worldt) * centerTranslation;
  updateCamera();
}

ospcommon::vec3f ArcballCamera::eyePos() const
{
  return xfmPoint(invCamera, ospcommon::vec3f(0, 0, 1));
}

ospcommon::vec3f ArcballCamera::center() const
{
  return -centerTranslation.p;
}

ospcommon::vec3f ArcballCamera::lookDir() const
{
  return xfmVector(invCamera, ospcommon::vec3f(0, 0, 1));
}

ospcommon::vec3f ArcballCamera::upDir() const
{
  return xfmVector(invCamera, ospcommon::vec3f(0, 1, 0));
}

void ArcballCamera::updateCamera()
{
  const ospcommon::AffineSpace3f rot    = ospcommon::LinearSpace3f(rotation);
  const ospcommon::AffineSpace3f camera = translation * rot * centerTranslation;
  invCamera                             = rcp(camera);
}

void ArcballCamera::updateWindowSize(const ospcommon::vec2i &windowSize)
{
  invWindowSize = ospcommon::vec2f(1) / ospcommon::vec2f(windowSize);
}

ospcommon::Quaternion3f ArcballCamera::screenToArcball(
    const ospcommon::vec2f &p)
{
  const float dist = dot(p, p);
  // If we're on/in the sphere return the point on it
  if (dist <= 1.f) {
    return ospcommon::Quaternion3f(0, p.x, p.y, std::sqrt(1.f - dist));
  } else {
    // otherwise we project the point onto the sphere
    const ospcommon::vec2f unitDir = normalize(p);
    return ospcommon::Quaternion3f(0, unitDir.x, unitDir.y, 0);
  }
}
