// Copyright 2017-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/math/AffineSpace.h"

class ArcballCamera
{
 public:
  ArcballCamera(const rkcommon::math::box3f &worldBounds,
      const rkcommon::math::vec2i &windowSize);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(
      const rkcommon::math::vec2f &from, const rkcommon::math::vec2f &to);
  void zoom(float amount);
  void pan(const rkcommon::math::vec2f &delta);

  rkcommon::math::vec3f eyePos() const;
  rkcommon::math::vec3f center() const;
  rkcommon::math::vec3f lookDir() const;
  rkcommon::math::vec3f upDir() const;

  void setRotation(rkcommon::math::quaternionf);

  void updateWindowSize(const rkcommon::math::vec2i &windowSize);

 protected:
  void updateCamera();

  // Project the point in [-1, 1] screen space onto the arcball sphere
  rkcommon::math::quaternionf screenToArcball(const rkcommon::math::vec2f &p);

  float zoomSpeed;
  rkcommon::math::vec2f invWindowSize;
  rkcommon::math::AffineSpace3f centerTranslation, translation, invCamera;
  rkcommon::math::quaternionf rotation;
};
