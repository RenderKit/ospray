/*
 *  *************************************************************
 *   Copyright 2016 Computational Engineering International, Inc.
 *   All Rights Reserved.
 *
 *        Restricted Rights Legend
 *
 *   Use, duplication, or disclosure of this
 *   software and its documentation by the
 *   Government is subject to restrictions as
 *   set forth in subdivision [(b)(3)(ii)] of
 *   the Rights in Technical Data and Computer
 *   Software clause at 52.227-7013.
 ***************************************************************/

#pragma once
#ifndef INC_MAPPING_H_
#define INC_MAPPING_H_

// care should be taken to make sure that the following defines are consistant
// in value with the defs. in common_uniforms.glsl (in library libGPURendering)
// !!!
enum InterpolationType
{
  Interpolated = 0,
  Banded = 1,
};

enum DisplayUndefinedType
{
  DISPUNDEFINED_BY_ZERO = 0,
  DISPUNDEFINED_BY_USERCOLOR = 1,
  DISPUNDEFINED_BY_INVISIBLE = 2,
};

enum LimitFringeType
{
  FRINGELIMIT_BY_NONE = 0,
  FRINGELIMIT_BY_PARTCOLOR = 1,
  FRINGELIMIT_BY_INVISIBLE = 2,
};

enum Palette1DInfo
{
  NLEVELS = 24,
};

// this structure is also used in the ISPC section
struct EnsightTex1dMappingData
{
  bool m_isAlphaTexture1D;
  bool m_paddings[3];
  float m_usercolor[4]; // display undefined by user color
  float m_partcolor[4]; // fringelimit by part color
  InterpolationType m_intp;
  DisplayUndefinedType m_dispundef;
  LimitFringeType m_limitfringe;
  int m_len;
  float m_levels[NLEVELS];
  float m_values[NLEVELS];
  float m_tmin; // for clammping t
  float m_tmax;
};


#ifdef __cplusplus
#define uniform
#define varying

// platform independent IEEE float math checks for inf, nan
#define FLT_SGN_MASK 0x80000000U
#define FLT_EXP_MASK 0x7F800000U
#define FLT_MAN_MASK 0x007FFFFFU
#define FLT_EXP_BIAS 127
#define FLT_EXP_SHIFT 23

#define FLT_ISFINITE(v) (((*((int *)&(v))) & FLT_EXP_MASK) != FLT_EXP_MASK)

#define FLT_ISNAN(v)                                                           \
  ((((*((int *)&(v))) & FLT_EXP_MASK) == FLT_EXP_MASK)                         \
      && (((*((int *)&(v))) & FLT_MAN_MASK) != 0U))

#define FLT_ISINF(v)                                                           \
  ((((*((int *)&(v))) & FLT_EXP_MASK) == FLT_EXP_MASK)                         \
      && (((*((int *)&(v))) & FLT_MAN_MASK) == 0U))

#define FLT_ISDENORM(v)                                                        \
  ((((*((int *)&(v))) & FLT_EXP_MASK) == 0U)                                   \
      && (((*((int *)&(v))) & FLT_MAN_MASK) != 0U))


inline float clamp(float x, float xmin, float xmax)
{
  if (x < xmin)
    x = xmin;
  else if (x > xmax)
    x = xmax;
  return x;
}

class EnsightTex1dMapping
{
 public:
  EnsightTex1dMapping(const char *s);
  EnsightTex1dMapping(const float partcolor[4],
      int intp,
      int dispundef,
      int limitfringe,
      int len,
      const float *levels,
      const float *values,
      float tmin,
      float tmax);
  void fromString(const char *s);
  inline bool isBanded() const
  {
    return d.m_intp == Banded;
  }
  inline void setIsAlphaTexture1D(bool f)
  {
    d.m_isAlphaTexture1D = f;
  }
  inline bool getIsAlphaTexture1D() const
  {
    return d.m_isAlphaTexture1D;
  }

  // return the t value, or, just use the part color
  float getT(float f, bool *usergba, float rgba[4]) const;

public:
  EnsightTex1dMappingData d;
};
#endif

#ifdef ISPC
typedef bool<2> bvec2;
typedef int<2> ivec2;
typedef float<3> vec3;
#define FLT_ISNAN isnan
#endif


inline float EnsightTex1dMapping_getT(
    const EnsightTex1dMappingData *d,
    float x,
    varying bool *usergba,
    varying float *rgba)
{
  *usergba = false;
  bool hasnan = false;
  if (FLT_ISNAN(x)) {
    if (DISPUNDEFINED_BY_USERCOLOR == d->m_dispundef) {
      *usergba = true;
      for (int i = 0; i < 4; i++)
        rgba[i] = d->m_usercolor[i];
      return 0;
    }
    if (DISPUNDEFINED_BY_INVISIBLE == d->m_dispundef) {
      // cannot handle INVISIBLE yet!
      return 0;
    }
    hasnan = true;
    x = 0;
  }

  const int len1 = d->m_len - 1;
  const float x0 = d->m_levels[0], x1 = d->m_levels[len1];
  float clampedx = clamp(x, x0, x1);
  if (!hasnan && x != clampedx) {
    if (FRINGELIMIT_BY_PARTCOLOR == d->m_limitfringe) {
      *usergba = true;
      for (int i = 0; i < 4; i++)
        rgba[i] = d->m_partcolor[i];
      return 0;
    } else if (FRINGELIMIT_BY_INVISIBLE == d->m_limitfringe) {
      // cannot handle INVISIBLE yet!
      return 0;
    }
  }
  x = clampedx;

  float t;
  if (x <= x0) {
    t = d->m_values[0];
  } else if (x >= x1) {
    t = d->m_values[len1];
  } else {
    int low = 0, high = len1;
    while (high - low > 1) {
      const int mid = (low + high) >> 1;
      if (x >= d->m_levels[mid]) {
        low = mid;
      } else {
        high = mid;
      }
    }
    t = d->m_values[low];
    if (high != low) {
      const float t0 = (x - d->m_levels[low]) / (d->m_levels[high] - d->m_levels[low]);
      t += (d->m_values[high] - d->m_values[low]) * t0;
    }
  }

  return clamp(t, d->m_tmin, d->m_tmax);
}



#endif
