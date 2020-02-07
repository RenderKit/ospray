// Copyright 2017-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ArcballCamera.h"

ArcballCamera::ArcballCamera(const ospcommon::math::box3f &worldBounds,
    const ospcommon::math::vec2i &windowSize)
    : zoomSpeed(1),
      invWindowSize(
          ospcommon::math::vec2f(1.0) / ospcommon::math::vec2f(windowSize)),
      centerTranslation(ospcommon::math::one),
      translation(ospcommon::math::one),
      rotation(ospcommon::math::one)
{
  ospcommon::math::vec3f diag = worldBounds.size();
  zoomSpeed = ospcommon::math::max(length(diag) / 150.0, 0.001);
  diag =
      ospcommon::math::max(diag, ospcommon::math::vec3f(0.3f * length(diag)));

  centerTranslation =
      ospcommon::math::AffineSpace3f::translate(-worldBounds.center());
  translation = ospcommon::math::AffineSpace3f::translate(
      ospcommon::math::vec3f(0, 0, length(diag)));
  updateCamera();
}

void ArcballCamera::rotate(
    const ospcommon::math::vec2f &from, const ospcommon::math::vec2f &to)
{
  rotation = screenToArcball(to) * screenToArcball(from) * rotation;
  updateCamera();
}

void ArcballCamera::zoom(float amount)
{
  amount *= zoomSpeed;
  translation = ospcommon::math::AffineSpace3f::translate(
                    ospcommon::math::vec3f(0, 0, amount))
      * translation;
  updateCamera();
}

void ArcballCamera::pan(const ospcommon::math::vec2f &delta)
{
  const ospcommon::math::vec3f t = ospcommon::math::vec3f(
      -delta.x * invWindowSize.x, delta.y * invWindowSize.y, 0);
  const ospcommon::math::vec3f worldt =
      translation.p.z * xfmVector(invCamera, t);
  centerTranslation =
      ospcommon::math::AffineSpace3f::translate(worldt) * centerTranslation;
  updateCamera();
}

ospcommon::math::vec3f ArcballCamera::eyePos() const
{
  return xfmPoint(invCamera, ospcommon::math::vec3f(0, 0, 1));
}

ospcommon::math::vec3f ArcballCamera::center() const
{
  return -centerTranslation.p;
}

ospcommon::math::vec3f ArcballCamera::lookDir() const
{
  return xfmVector(invCamera, ospcommon::math::vec3f(0, 0, 1));
}

ospcommon::math::vec3f ArcballCamera::upDir() const
{
  return xfmVector(invCamera, ospcommon::math::vec3f(0, 1, 0));
}

void ArcballCamera::updateCamera()
{
  const ospcommon::math::AffineSpace3f rot =
      ospcommon::math::LinearSpace3f(rotation);
  const ospcommon::math::AffineSpace3f camera =
      translation * rot * centerTranslation;
  invCamera = rcp(camera);
}

void ArcballCamera::setRotation(ospcommon::math::quaternionf q)
{
  rotation = q;
  updateCamera();
}

void ArcballCamera::updateWindowSize(const ospcommon::math::vec2i &windowSize)
{
  invWindowSize =
      ospcommon::math::vec2f(1) / ospcommon::math::vec2f(windowSize);
}

ospcommon::math::quaternionf ArcballCamera::screenToArcball(
    const ospcommon::math::vec2f &p)
{
  const float dist = dot(p, p);
  // If we're on/in the sphere return the point on it
  if (dist <= 1.f) {
    return ospcommon::math::quaternionf(0, p.x, p.y, std::sqrt(1.f - dist));
  } else {
    // otherwise we project the point onto the sphere
    const ospcommon::math::vec2f unitDir = normalize(p);
    return ospcommon::math::quaternionf(0, unitDir.x, unitDir.y, 0);
  }
}
