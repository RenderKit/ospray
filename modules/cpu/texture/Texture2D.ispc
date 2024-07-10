// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.ih"
#include "Texture2DUtil.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

// TODO tiling

// Texture coordinate utilities
//////////////////////////////////////////////////////////////////////////////

inline float wrap_coord(
    const uniform uint32 wrapMode, const float halfTexel, const float p)
{
  float tc = 0.f;

  switch (wrapMode) {
  case OSP_TEXTURE_WRAP_REPEAT:
    tc = frac(p - halfTexel) + halfTexel;
    tc = max(tc, 0.f); // filter out inf/NaN
    break;

  case OSP_TEXTURE_WRAP_MIRRORED_REPEAT: {
    tc = 2.f * frac(p * 0.5f);
    if (tc >= 1.f)
      tc = 2.f - tc;
    tc = clamp(tc, halfTexel, 1.f - halfTexel);
    break;
  }

  case OSP_TEXTURE_WRAP_CLAMP_TO_EDGE:
    tc = clamp(p, halfTexel, 1.f - halfTexel);
  }

  return tc;
}

inline vec2i nearest_coords(
    const vec2f p, const uniform vec2ui wrapMode, const vec2i size)
{
  vec2f tc;

  tc.x = wrap_coord(wrapMode.x, 0.f, p.x);
  tc.y = wrap_coord(wrapMode.y, 0.f, p.y);

  // scale by texture size and clamp
  const vec2i tci = min(make_vec2i(tc * make_vec2f(size.x, size.y)), size - 1);
  return tci;
}

struct BilinCoords
{
  vec2i st0;
  vec2i st1;
  vec2f frac;
};

inline BilinCoords bilinear_coords(
    const vec2f p, const uniform vec2ui wrapMode, const vec2i size)
{
  BilinCoords coords;
  vec2f tc;

  const vec2f sizef = make_vec2f(size.x, size.y);
  const vec2f halfTexel = rcp(sizef) * 0.5f;

  tc.x = wrap_coord(wrapMode.x, halfTexel.x, p.x);
  tc.y = wrap_coord(wrapMode.y, halfTexel.y, p.y);

  tc = tc - halfTexel; // lower sample shifted by half a texel
  tc = tc * sizef;

  coords.frac = frac(tc);
  coords.st0 = min(make_vec2i(tc), size - 1);
  coords.st1 = coords.st0 + 1;

  // handle border cases as REPEAT
  if (coords.st1.x >= size.x)
    coords.st1.x = 0;
  if (coords.st1.y >= size.y)
    coords.st1.y = 0;

  return coords;
}

inline vec4f bilerp(const vec2f frac,
    const vec4f c00,
    const vec4f c01,
    const vec4f c10,
    const vec4f c11)
{
  return lerp(frac.y, lerp(frac.x, c00, c01), lerp(frac.x, c10, c11));
}

inline vec2i sizeAtLevel(const vec2i size, const int level)
{
  return make_vec2i(max(size.x >> level, 1), max(size.y >> level, 1));
}

#define __define_tex_get(FMT)                                                  \
                                                                               \
  static vec4f Texture2D_nearest_l_##FMT(const Texture2D *uniform self,        \
      const DifferentialGeometry &dg,                                          \
      const int level)                                                         \
  {                                                                            \
    const vec2i size = sizeAtLevel(self->size, level);                         \
    const vec2i i = nearest_coords(dg.st, self->wrapMode, size);               \
    const uint32 idx = i.y * size.x + i.x;                                     \
    return getTexel_##FMT(self->data[level], idx);                             \
  }                                                                            \
                                                                               \
  static vec4f Texture2D_bilinear_l_##FMT(const Texture2D *uniform self,       \
      const DifferentialGeometry &dg,                                          \
      const int level)                                                         \
  {                                                                            \
    const vec2i size = sizeAtLevel(self->size, level);                         \
    const BilinCoords cs = bilinear_coords(dg.st, self->wrapMode, size);       \
    const void *data = self->data[level];                                      \
                                                                               \
    const vec4f c00 = getTexel_##FMT(data, cs.st0.y * size.x + cs.st0.x);      \
    const vec4f c01 = getTexel_##FMT(data, cs.st0.y * size.x + cs.st1.x);      \
    const vec4f c10 = getTexel_##FMT(data, cs.st1.y * size.x + cs.st0.x);      \
    const vec4f c11 = getTexel_##FMT(data, cs.st1.y * size.x + cs.st1.x);      \
                                                                               \
    return bilerp(cs.frac, c00, c01, c10, c11);                                \
  }                                                                            \
                                                                               \
  static vec4f Texture2D_##FMT(const Texture2D *uniform self,                  \
      const DifferentialGeometry &dg,                                          \
      const int level,                                                         \
      const float frac,                                                        \
      const bool filter_nearest)                                               \
  {                                                                            \
    vec4f t0 = filter_nearest ? Texture2D_nearest_l_##FMT(self, dg, level)     \
                              : Texture2D_bilinear_l_##FMT(self, dg, level);   \
    if (frac == 0.f)                                                           \
      return t0;                                                               \
                                                                               \
    vec4f t1 = filter_nearest                                                  \
        ? Texture2D_nearest_l_##FMT(self, dg, level + 1)                       \
        : Texture2D_bilinear_l_##FMT(self, dg, level + 1);                     \
    return lerp(frac, t0, t1);                                                 \
  }                                                                            \
  struct swallow_semicolon

__foreach_fetcher(__define_tex_get);

#define __define_tex_getN(FMT, C)                                              \
  static vec3f Texture2D_N_##FMT(const Texture2D *uniform self,                \
      const DifferentialGeometry &dg,                                          \
      const int level,                                                         \
      const float frac,                                                        \
      const bool filter_nearest)                                               \
  {                                                                            \
    return make_vec3f(Texture2D_##FMT(self, dg, level, frac, filter_nearest))  \
        * C                                                                    \
        - 1.f;                                                                 \
  }                                                                            \
  struct swallow_semicolon

#define __define_tex_case(NAME, FMT)                                           \
  case OSP_TEXTURE_##FMT:                                                      \
    return (void *uniform) & NAME##_##FMT

#define __define_tex_call_tex_case(FMT) __define_call_tex_case(Texture2D, FMT)
#define __define_tex_callN_tex_case(FMT)                                       \
  __define_call_tex_case(Texture2D_N, FMT)

#define __define_tex_get_case(FMT) __define_tex_case(Texture2D, FMT)
#define __define_tex_getN_case(FMT) __define_tex_case(Texture2D_N, FMT)

#define __define_call_tex_case(NAME, FMT)                                      \
  case OSP_TEXTURE_##FMT:                                                      \
    return NAME##_##FMT(self, dg, mipLevel, frac, filter_nearest)

export void *uniform Texture2D_get_addr(const uniform uint32 type)
{
  switch (type) {
    __foreach_fetcher(__define_tex_get_case);
  default:
    break;
  }
  return NULL;
};

inline float calcLambda(const float pixelFootprint,
    const vec2i size,
    const uniform bool filter_nearest)
{
  float t0 = pixelFootprint * max(size.x, size.y);
  if (!filter_nearest)
    t0 *= 0.5f; // bilinear filtering increases filter width by 2: compensate
  return log(t0) / log(2.f);
}

SYCL_EXTERNAL vec4f Texture2D_get(
    const Texture *uniform _self, const DifferentialGeometry &dg)
{
  const Texture2D *uniform self = (const Texture2D *uniform)_self;

  int mipLevel = 0;
  float frac = 0.f;
  const uniform bool filter_nearest = self->filter & OSP_TEXTURE_FILTER_NEAREST;
  if (self->maxLevel) {
    const float lambda =
        max(0.f, calcLambda(dg.pixelFootprint, self->size, filter_nearest));
    mipLevel = min((int)lambda, self->maxLevel - 1);
    frac = min(lambda - mipLevel, 1.f);
  }
  switch (self->format) {
    __foreach_fetcher(__define_tex_call_tex_case);
  default:
    break;
  }
  return make_vec4f(0.f, 0.f, 0.f, 1.f);
}

static vec3f Texture2D_Normal_neutral(
    const Texture2D *uniform, const DifferentialGeometry &)
{
  return make_vec3f(0.f, 0.f, 1.f);
}

__define_tex_getN(RGB8, (255.f / 127.f));
__define_tex_getN(RGBA8, (255.f / 127.f));
__define_tex_getN(RGB32F, 2.f);
__define_tex_getN(RGBA32F, 2.f);
__define_tex_getN(RGB16F, 2.f);
__define_tex_getN(RGBA16F, 2.f);
__define_tex_getN(RGB16, (65535.f / 32767.f));
__define_tex_getN(RGBA16, (65535.f / 32767.f));

export void *uniform Texture2D_getN_addr(const uniform uint32 type)
{
  switch (type) {
  case OSP_TEXTURE_SRGBA: /* fallthrough, sRGB ignored for normals */
    __define_tex_getN_case(RGBA8);
  case OSP_TEXTURE_SRGB: /* fallthrough, sRGB ignored for normals */
    __define_tex_getN_case(RGB8);
    __define_tex_getN_case(RGBA32F);
    __define_tex_getN_case(RGB32F);
    __define_tex_getN_case(RGBA16F);
    __define_tex_getN_case(RGB16F);
    __define_tex_getN_case(RGBA16);
    __define_tex_getN_case(RGB16);
  default:
    break;
  }
  return (void *uniform) & Texture2D_Normal_neutral;
};

SYCL_EXTERNAL vec3f Texture2D_getNormal(
    const Texture *uniform _self, const DifferentialGeometry &dg)
{
  const Texture2D *uniform self = (const Texture2D *uniform)_self;

  int mipLevel = 0;
  float frac = 0.f;
  const uniform bool filter_nearest = self->filter & OSP_TEXTURE_FILTER_NEAREST;
  if (self->maxLevel) {
    const float lambda =
        max(0.f, calcLambda(dg.pixelFootprint, self->size, filter_nearest));
    mipLevel = min((int)lambda, self->maxLevel - 1);
    frac = min(lambda - mipLevel, 1.f);
  }
  switch (self->format) {
  case OSP_TEXTURE_SRGBA: /* fallthrough, sRGB ignored for normals */
    __define_tex_callN_tex_case(RGBA8);
  case OSP_TEXTURE_SRGB: /* fallthrough, sRGB ignored for normals */
    __define_tex_callN_tex_case(RGB8);
    __define_tex_callN_tex_case(RGBA32F);
    __define_tex_callN_tex_case(RGB32F);
    __define_tex_callN_tex_case(RGBA16F);
    __define_tex_callN_tex_case(RGB16F);
    __define_tex_callN_tex_case(RGBA16);
    __define_tex_callN_tex_case(RGB16);
  default:
    break;
  }
  return Texture2D_Normal_neutral(self, dg);
}

// Poor mans unit tests
//////////////////////////////////////////////////////////////////////////////

#ifdef OSPRAY_UNITTEST

uniform size_t channels(OSPTextureFormat format)
{
  switch (format) {
  case OSP_TEXTURE_RGBA8:
  case OSP_TEXTURE_SRGBA:
  case OSP_TEXTURE_RGBA32F:
  case OSP_TEXTURE_RGBA16:
  case OSP_TEXTURE_RGBA16F:
    return 4;
  case OSP_TEXTURE_RGB8:
  case OSP_TEXTURE_SRGB:
  case OSP_TEXTURE_RGB32F:
  case OSP_TEXTURE_RGB16:
  case OSP_TEXTURE_RGB16F:
    return 3;
  case OSP_TEXTURE_RA32F:
  case OSP_TEXTURE_RA8:
  case OSP_TEXTURE_LA8:
  case OSP_TEXTURE_RA16:
  case OSP_TEXTURE_RA16F:
    return 2;
  case OSP_TEXTURE_R8:
  case OSP_TEXTURE_L8:
  case OSP_TEXTURE_R32F:
  case OSP_TEXTURE_R16:
  case OSP_TEXTURE_R16F:
    return 1;
  default:
    return 0;
  }
}

uniform bool hdr(OSPTextureFormat format)
{
  switch (format) {
  case OSP_TEXTURE_RGBA32F:
  case OSP_TEXTURE_RGBA16F:
  case OSP_TEXTURE_RGB32F:
  case OSP_TEXTURE_RGB16F:
  case OSP_TEXTURE_RA32F:
  case OSP_TEXTURE_RA16F:
  case OSP_TEXTURE_R32F:
  case OSP_TEXTURE_R16F:
    return true;
  default:
    return false;
  }
}

#define define_texAccessor_consistency(FMT)                                    \
  {                                                                            \
    setTexel_##FMT(&data, programIndex, testIn);                               \
    const vec4f testOut = getTexel_##FMT(&data, programIndex);                 \
    const uniform int ch = channels(OSP_TEXTURE_##FMT);                        \
    const uniform bool alpha = ch == 2 || ch == 4;                             \
    const uniform bool hd = hdr(OSP_TEXTURE_##FMT);                            \
    const uniform bool lm = OSP_TEXTURE_##FMT == OSP_TEXTURE_LA8               \
        || OSP_TEXTURE_##FMT == OSP_TEXTURE_L8;                                \
    if (ch == 0)                                                               \
      print("Invalid format " #FMT "\n");                                      \
    if (abs(testIn.x - testOut.x) > 1e-2f && (hd || testOut.x != 1.0f)) {      \
      print("Failed ch 1 and format " #FMT " % %\n", testIn.x, testOut.x);     \
      success = false;                                                         \
    }                                                                          \
    if (abs(testIn.y - testOut.y) > 1e-2f && (hd || testOut.y != 1.0f)         \
        && (ch > 2 || testOut.y != 0.0f) && (!lm || testOut.y != testOut.x)) { \
      print("Failed ch 2 and format " #FMT " % %\n", testIn.y, testOut.y);     \
      success = false;                                                         \
    }                                                                          \
    if (abs(testIn.z - testOut.z) > 1e-2f && (hd || testOut.z != 1.0f)         \
        && (ch > 2 || testOut.z != 0.0f) && (!lm || testOut.z != testOut.x)) { \
      print("Failed ch 3 and format " #FMT " % %\n", testIn.z, testOut.z);     \
      success = false;                                                         \
    }                                                                          \
    if (abs(testIn.w - testOut.w) > 1e-2f                                      \
        && (hd && alpha || testOut.w != 1.0f)) {                               \
      print("Failed ch 4 and format " #FMT " % %\n", testIn.w, testOut.w);     \
      success = false;                                                         \
    }                                                                          \
  }                                                                            \
  struct swallow_semicolon

bool checkTexAcc(const vec4f testIn)
{
  bool success = true;
  vec4f data; // largest format

  __foreach_fetcher(define_texAccessor_consistency);

  return success;
}

export uniform bool checkTextureAccessorConsistency()
{
  bool success = true;
  success &= checkTexAcc(make_vec4f(programIndex * 0.15f));
  success &= checkTexAcc(make_vec4f(0.4f + programIndex * 0.027f));
  success &= checkTexAcc(make_vec4f(0.01f, 0.91f, 0.37f, 0.5f));
  return all(success);
}
#endif

OSPRAY_END_ISPC_NAMESPACE
