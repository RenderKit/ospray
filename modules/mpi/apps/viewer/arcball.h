#pragma once

#include <ospcommon/AffineSpace.h>

struct Arcball {
  Arcball(const ospcommon::box3f &worldBounds);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(const ospcommon::vec2f &from, const ospcommon::vec2f &to);
  void zoom(float amount);
  ospcommon::vec3f eyePos() const;
  ospcommon::vec3f lookDir() const;
  ospcommon::vec3f upDir() const;

private:
  void updateCamera();
  // Project the point in [-1, 1] screen space onto the arcball sphere
  ospcommon::Quaternion3f screenToArcball(const ospcommon::vec2f &p);

  float zoomSpeed;
  ospcommon::AffineSpace3f lookAt, translation, inv_camera;
  ospcommon::Quaternion3f rotation;
};

