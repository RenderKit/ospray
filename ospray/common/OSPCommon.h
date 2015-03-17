// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#pragma once

/*! \file OSPCommon.h Defines common types and classes that _every_
  ospray file should know about */

#ifdef __WIN32__
# define _USE_MATH_DEFINES 1
# include <cmath>
# include <math.h>
# ifndef M_PI
#  define M_PI       3.14159265358979323846
# endif
#endif


// embree
#include "common/math/vec2.h"
#include "common/math/vec3.h"
#include "common/math/vec4.h"
#include "common/math/bbox.h"
#include "common/math/affinespace.h"
#include "common/sys/ref.h"
#include "common/sys/taskscheduler.h"

// ospray
#include "ospray/common/OSPDataType.h"

// std
#include <stdint.h> // for int64_t etc


#ifdef OSPRAY_TARGET_MIC
inline void* operator new(size_t size) throw(std::bad_alloc) { return embree::alignedMalloc(size); }       
inline void operator delete(void* ptr) throw() { embree::alignedFree(ptr); }      
inline void* operator new[](size_t size) throw(std::bad_alloc) { return embree::alignedMalloc(size); }  
inline void operator delete[](void* ptr) throw() { embree::alignedFree(ptr); }    
#endif

//! main namespace for all things ospray (for internal code)
namespace ospray {

  using embree::one;
  using embree::empty;
  using embree::zero;

  /*! basic types */
  typedef ::int64_t int64;
  typedef ::uint64_t uint64;

  typedef ::int32_t int32;
  typedef ::uint32_t uint32;

  typedef ::int16_t int16;
  typedef ::uint16_t uint16;

  typedef ::int8_t int8;
  typedef ::uint8_t uint8;

  /*! OSPRay's two-int vector class */
  typedef embree::Vec2i    vec2i;
  /*! OSPRay's three-unsigned char vector class */
  typedef embree::Vec3<uint8> vec3uc;
  /*! OSPRay's 2x uint32 vector class */
  typedef embree::Vec2<uint32> vec2ui;
  /*! OSPRay's 3x uint32 vector class */
  typedef embree::Vec3<uint32> vec3ui;
  /*! OSPRay's 4x uint32 vector class */
  typedef embree::Vec4<uint32> vec4ui;
  /*! OSPRay's 3x int32 vector class */
  typedef embree::Vec3<int32>  vec3i;
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

  typedef embree::BBox<vec2ui>   box2ui;
  typedef embree::BBox<vec2i>    region2i;
  typedef embree::BBox<vec2ui>   region2ui;
  
  typedef embree::BBox3f         box3f;
  typedef embree::BBox3fa        box3fa;
  typedef embree::BBox<vec3uc>   box3uc;
  typedef embree::BBox<vec4f>    box4f;
  typedef embree::BBox3fa        box3fa;
  
  /*! affice space transformation */
  typedef embree::AffineSpace3f  affine3f;
  typedef embree::AffineSpace3fa affine3fa;
  typedef embree::AffineSpace3f  AffineSpace3f;
  typedef embree::AffineSpace3fa AffineSpace3fa;

  typedef embree::LinearSpace3f  linear3f;
  typedef embree::LinearSpace3fa linear3fa;
  typedef embree::LinearSpace3f  LinearSpace3f;
  typedef embree::LinearSpace3fa LinearSpace3fa;

  using   embree::Ref;
  using   embree::RefCount;

  /*! return system time in seconds */
  double getSysTime();

  void init(int *ac, const char ***av);

  /*! remove specified num arguments from an ac/av arglist */
  void removeArgs(int &ac, char **&av, int where, int howMany);
  /*! for debugging. compute a checksum for given area range... */
  void *computeCheckSum(const void *ptr, size_t numBytes);

#ifdef NDEBUG
# define Assert(expr) /* nothing */
# define Assert2(expr,expl) /* nothing */
# define AssertError(errMsg) /* nothing */
#else
  extern void doAssertion(const char *file, int line, const char *expr, const char *expl);
# define Assert(expr)                                                   \
  ((void)((expr) ? 0 : ((void)ospray::doAssertion(__FILE__, __LINE__, #expr, NULL), 0)))
# define Assert2(expr,expl)                                             \
  ((void)((expr) ? 0 : ((void)ospray::doAssertion(__FILE__, __LINE__, #expr, expl), 0)))
# define AssertError(errMsg)                            \
  doAssertion(__FILE__,__LINE__, (errMsg), NULL)
#endif

  /*! logging level (cmdline: --osp:loglevel \<n\>) */
  extern uint32 logLevel;
  /*! whether we're running in debug mode (cmdline: --osp:debug) */
  extern bool debugMode;

  /*! error handling callback to be used by embree */
  //  void error_handler(const RTCError code, const char *str);

  /*! size of OSPDataType */
  size_t sizeOf(OSPDataType type);

  /*! Convert a type string to an OSPDataType. */
  OSPDataType typeForString(const char *string);

} // ::ospray

#define NOTIMPLEMENTED    throw std::runtime_error(std::string(__PRETTY_FUNCTION__)+": not implemented...");

#define divRoundUp(X,Y) (((X)+(Y)-1)/(Y))
  

