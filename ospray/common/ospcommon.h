/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \file ospcommon.h Defines common types and classes that _every_
    ospray file should know about */

// embree
#include "common/math/vec2.h"
#include "common/math/vec3.h"
#include "common/math/vec4.h"
#include "common/math/bbox.h"
#include "common/math/affinespace.h"
#include "common/sys/ref.h"
#include "common/sys/taskscheduler.h"
//#include "embree2/rtcore.h"


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
  
  /*! affice space transformation */
  typedef embree::AffineSpace3f  affine3f;
  typedef embree::AffineSpace3fa affine3fa;
  typedef embree::AffineSpace3f  AffineSpace3f;
  typedef embree::AffineSpace3fa AffineSpace3fa;

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
# define Assert(expr)                                                    \
  ((void)((expr) ? 0 : ((void)ospray::doAssertion(__FILE__, __LINE__, #expr, NULL), 0)))
# define Assert2(expr,expl)                                              \
  ((void)((expr) ? 0 : ((void)ospray::doAssertion(__FILE__, __LINE__, #expr, expl), 0)))
# define AssertError(errMsg)                     \
  doAssertion(__FILE__,__LINE__, (errMsg), NULL)
#endif

  /*! logging level (cmdline: --osp:loglevel \<n\>) */
  extern uint32 logLevel;
  /*! whether we're running in debug mode (cmdline: --osp:debug) */
  extern bool debugMode;

  /*! error handling callback to be used by embree */
  //  void error_handler(const RTCError code, const char *str);
}

#define NOTIMPLEMENTED    throw std::runtime_error(std::string(__PRETTY_FUNCTION__)+": not implemented...");

#define divRoundUp(X,Y) (((X)+(Y)-1)/(Y))
  

