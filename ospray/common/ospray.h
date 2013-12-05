#pragma once

#include "common/math/vec2.h"
#include "common/math/vec3.h"
#include "common/math/bbox.h"
#include "common/sys/ref.h"

namespace ospray {
  typedef unsigned char uchar;
  
  typedef embree::Vec2i    vec2i;
  typedef embree::Vec3i    vec3i;
  typedef embree::Vec3f    vec3f;
  typedef embree::Vec3fa   vec3fa;
  typedef embree::BBox3f   box3f;

  using   embree::Ref;

  /*! return system time in seconds */
  double getSysTime();

  void init(int *ac, const char ***av);
  void removeArgs(int &ac, char **&av, int where, int howMany);
}

#define NOTIMPLEMENTED    throw std::runtime_error(std::string(__PRETTY_FUNCTION__)+": not implemented...");


