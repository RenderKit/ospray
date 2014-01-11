#pragma once

/*! \file ospcommon.h Defines common types and classes that _every_
    ospray file should know about */

#include "common/math/vec2.h"
#include "common/math/vec3.h"
#include "common/math/vec4.h"
#include "common/math/bbox.h"
#include "common/math/affinespace.h"
#include "common/sys/ref.h"


//! main namespace for all things ospray (for internal code)
namespace ospray {

  /*! basic types */
  typedef          long long  int64;
  typedef unsigned long long uint64;
  typedef                int  int32;
  typedef unsigned       int uint32;
  typedef              short  int16;
  typedef unsigned     short uint16;
  typedef               char   int8;
  typedef unsigned      char  uint8;

  /*! OSPRay's two-int vector class */
  typedef embree::Vec2i    vec2i;
  /*! OSPRay's three-int vector class */
  typedef embree::Vec3i    vec3i;
  /*! OSPRay's four-int vector class */
  typedef embree::Vec4i    vec4i;
  /*! OSPRay's two-float vector class */
  typedef embree::Vec2f    vec2f;
  /*! OSPRay's three-float vector class */
  typedef embree::Vec3f    vec3f;
  /*! OSPRay's three-float vector class (aligned to 16b-boundaries) */
  typedef embree::Vec3fa   vec3fa;
  /*! OSPRay's four-float vector class */
  typedef embree::Vec4f    vec4f;

  typedef embree::BBox3f      box3f;
  
  /*! affice space transformation */
  typedef embree::AffineSpace3f affine3f;

  using   embree::Ref;
  using   embree::RefCount;

  /*! return system time in seconds */
  double getSysTime();

  void init(int *ac, const char ***av);

  /*! remove specified num arguments from an ac/av arglist */
  void removeArgs(int &ac, char **&av, int where, int howMany);

  
  extern void doAssertion(const char *file, int line, const char *expr, const char *expl);
#define Assert(expr)                                                    \
  ((void)((expr) ? 0 : ((void)ospray::doAssertion(__FILE__, __LINE__, #expr, NULL), 0)))
#define Assert2(expr,expl)                                              \
  ((void)((expr) ? 0 : ((void)ospray::doAssertion(__FILE__, __LINE__, #expr, expl), 0)))
#define AssertError(errMsg)                     \
  doAssertion(__FILE__,__LINE__, (errMsg), NULL)

  extern uint32 logLevel;
}

#define NOTIMPLEMENTED    throw std::runtime_error(std::string(__PRETTY_FUNCTION__)+": not implemented...");

