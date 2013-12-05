#pragma once

/*! \file ospray.h Defines common types and classes that _every_
    ospray file should know about */

#include "common/math/vec2.h"
#include "common/math/vec3.h"
#include "common/math/bbox.h"
#include "common/sys/ref.h"

//! main namespace for all things ospray
namespace ospray {
  typedef unsigned char uchar;
  /*! OSPRay's two-int vector class */
  typedef embree::Vec2i    vec2i;
  /*! OSPRay's three-int vector class */
  typedef embree::Vec3i    vec3i;
  /*! OSPRay's two-float vector class */
  typedef embree::Vec2f    vec2f;
  /*! OSPRay's three-float vector class */
  typedef embree::Vec3f    vec3f;
  /*! OSPRay's three-float vector class (aligned to 16b-boundaries) */
  typedef embree::Vec3fa   vec3fa;

  /*! OSPRay's bounding box type for 3D float space */
  typedef embree::BBox3f   box3f;

  using   embree::Ref;

  /*! return system time in seconds */
  double getSysTime();

  void init(int *ac, const char ***av);

  /*! remove specified num arguments from an ac/av arglist */
  void removeArgs(int &ac, char **&av, int where, int howMany);
}

#define NOTIMPLEMENTED    throw std::runtime_error(std::string(__PRETTY_FUNCTION__)+": not implemented...");


