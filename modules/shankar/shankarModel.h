#pragma once

#include "ospray/common/model.h"

namespace ospray {
  namespace shankar {
    struct Point {
      vec3f pos;
      vec3f nor;
      float rad;
    };

    struct Surface {
      std::vector<Point> point;

      void load(const std::string &fn);
      /*! for when we are using the adamson/alexa model */
      void computeNormalsAndRadii();

      box3f getBounds() const;
    };

    struct Model {
      std::vector<Surface *> surface;
      box3f getBounds() const;
    };
  }
}

