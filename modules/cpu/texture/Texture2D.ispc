// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.ih"
#include "Texture2DFormats.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE
#ifdef OSPRAY_TARGET_SYCL
using half = sycl::half;
namespace syclexp = sycl::ext::oneapi::experimental;
#endif
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

inline float calcLambda(const float pixelFootprint,
    const vec2i size,
    const uniform bool filter_nearest)
{
  float t0 = pixelFootprint * max(size.x, size.y);
  if (!filter_nearest)
    t0 *= 0.5f; // bilinear filtering increases filter width by 2: compensate
  return log(t0) / log(2.f);
}

inline vec4f Texture2D_nearest_l(
    const Texture2D *uniform self, const vec2f &st, const int level)
{
  const vec2i size = sizeAtLevel(self->size, level);
  const vec2i i = nearest_coords(st, self->wrapMode, size);
  const uint32 idx = i.y * size.x + i.x;
  return Texture2D_getTexel(self->format, self->data[level], idx);
}

inline vec4f Texture2D_bilinear_l(
    const Texture2D *uniform self, const vec2f &st, const int level)
{
  const vec2i size = sizeAtLevel(self->size, level);
  const BilinCoords cs = bilinear_coords(st, self->wrapMode, size);
  const void *data = self->data[level];

  const vec4i idx = make_vec4i(cs.st0.y * size.x + cs.st0.x,
      cs.st0.y * size.x + cs.st1.x,
      cs.st1.y * size.x + cs.st0.x,
      cs.st1.y * size.x + cs.st1.x);
  const Vec4fPack vp = Texture2D_getTexel4(self->format, data, idx);
  return bilerp(cs.frac, vp.v1, vp.v2, vp.v3, vp.v4);
}
#ifdef OSPRAY_TARGET_SYCL
inline sycl::float4 fixChannelFormat(sycl::float4 s, OSPTextureFormat format) {
  if (format == OSP_TEXTURE_R8 || format == OSP_TEXTURE_L8 
      || format == OSP_TEXTURE_R32F || format == OSP_TEXTURE_R16 
      || format == OSP_TEXTURE_R16F) {
    return sycl::float4(s.x(), s.x(), s.x(), 1.f);
  } else if (format == OSP_TEXTURE_RA8 || format == OSP_TEXTURE_LA8
      || format == OSP_TEXTURE_RA16 || format == OSP_TEXTURE_RA32F
      || format == OSP_TEXTURE_RA16F) {
    return sycl::float4(s.x(), s.x(), s.x(), s.y());
  }
  return s;
}
#endif

SYCL_EXTERNAL __noinline vec4f Texture2D_get(
    const Texture2D *uniform self, const vec2f &st, const float pixelFootprint)
{
 #ifdef OSPRAY_TARGET_SYCL
  const bool filter_nearest = self->filter & OSP_TEXTURE_FILTER_NEAREST;
  int level0 = 0;
  int level1 = 0;
  float frac = 0.f;

  if (self->maxLevel) {
    const float lambda = max(0.f, calcLambda(pixelFootprint, self->size, filter_nearest));
    level0 = min((int)lambda, self->maxLevel - 1);
    level1 = min(level0 + 1, self->maxLevel);
    frac = min(lambda - (float)level0, 1.f);
  }

  syclexp::sampled_image_handle handle0(
      (syclexp::sampled_image_handle::raw_image_handle_type)self->data[level0]);
  sycl::float4 s0 = syclexp::sample_image<sycl::float4>(
      handle0, sycl::float2{st.x, st.y});
  s0 = fixChannelFormat(s0, self->format);

  if (frac == 0.f)
    return make_vec4f(s0.x(), s0.y(), s0.z(), s0.w());

  syclexp::sampled_image_handle handle1(
      (syclexp::sampled_image_handle::raw_image_handle_type)self->data[level1]);
  sycl::float4 s1 = syclexp::sample_image<sycl::float4>(
      handle1, sycl::float2{st.x, st.y});
  s1 = fixChannelFormat(s1, self->format);

  sycl::float4 res = s0 + frac * (s1 - s0);
  return make_vec4f(res.x(), res.y(), res.z(), res.w());
#else
  int mipLevel = 0;
  float frac = 0.f;
  const uniform bool filter_nearest = self->filter & OSP_TEXTURE_FILTER_NEAREST;
  if (self->maxLevel) {
    const float lambda =
        max(0.f, calcLambda(pixelFootprint, self->size, filter_nearest));
    mipLevel = min((int)lambda, self->maxLevel - 1);
    frac = min(lambda - mipLevel, 1.f);
  }
  vec4f t0 = filter_nearest ? Texture2D_nearest_l(self, st, mipLevel)
                            : Texture2D_bilinear_l(self, st, mipLevel);
  if (frac == 0.f)
    return t0;
  vec4f t1 = filter_nearest ? Texture2D_nearest_l(self, st, mipLevel + 1)
                            : Texture2D_bilinear_l(self, st, mipLevel + 1);
  return lerp(frac, t0, t1);
#endif
}

export void *uniform Texture2D_get_addr(const uniform uint32 type)
{
  return (void *uniform)Texture2D_get;
}

OSPRAY_END_ISPC_NAMESPACE
