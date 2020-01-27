// Copyright 2017-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospcommon/math/AffineSpace.h"

class ArcballCamera
{
 public:
  ArcballCamera(const ospcommon::math::box3f &worldBounds,
      const ospcommon::math::vec2i &windowSize);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(
      const ospcommon::math::vec2f &from, const ospcommon::math::vec2f &to);
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
  ospcommon::math::quaternionf screenToArcball(const ospcommon::math::vec2f &p);

  float zoomSpeed;
  ospcommon::math::vec2f invWindowSize;
  ospcommon::math::AffineSpace3f centerTranslation, translation, invCamera;
  ospcommon::math::quaternionf rotation;
};
