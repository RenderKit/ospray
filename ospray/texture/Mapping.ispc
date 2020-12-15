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
#include "Mapping.ih"

EnsightTex1dMappingOut EnsightTex1dMapping_getT(const EnsightTex1dMappingData *d, float x)
{
  EnsightTex1dMappingOut out;
  out.usergba = false;
  out.t = 0;
  out.interpl = (d->m_intp != Banded);
  bool hasnan = false;
  if (FLT_ISNAN(x)) {
    if (DISPUNDEFINED_BY_USERCOLOR == d->m_dispundef) {
      out.usergba = true;
      for (int i = 0; i < 4; i++) {
        out.rgba[i] = d->m_usercolor[i];
      }
      return out;
    }
    if (DISPUNDEFINED_BY_INVISIBLE == d->m_dispundef) {
      // cannot handle INVISIBLE yet!
      return out;
    }
    hasnan = true;
    x = 0;
  }

  const int len1 = d->m_len - 1;
  const float x0 = d->m_levels[0], x1 = d->m_levels[len1];
  const float clampedx = clamp(x, x0, x1);
  if (!hasnan && x != clampedx) {
    if (FRINGELIMIT_BY_PARTCOLOR == d->m_limitfringe) {
      out.usergba = true;
      for (int i = 0; i < 4; i++) {
        out.rgba[i] = d->m_partcolor[i];
      }
      return out;
    }
    if (FRINGELIMIT_BY_INVISIBLE == d->m_limitfringe) {
      // cannot handle INVISIBLE yet!
      return out;
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
    const float t0 = (x - d->m_levels[low]) / (d->m_levels[high] - d->m_levels[low]);
    t += (d->m_values[high] - d->m_values[low]) * t0;
  }

  out.t = clamp(t, d->m_tmin, d->m_tmax);
  return out;
}
