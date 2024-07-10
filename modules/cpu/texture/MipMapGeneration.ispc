// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2DUtil.ih"
#include "ospray/OSPEnums.h"

#define __define_tex_filterkernel_case(FMT)                                    \
  case OSP_TEXTURE_##FMT:                                                      \
    foreach (x = 0 ... dstSize.x) {                                            \
      const float tx = x * r.x;                                                \
      const int sx = 2 * x;                                                    \
      const vec4f v0 = (h.x - tx) * getTexel_##FMT(src, srcy + sx)             \
          + h.x * getTexel_##FMT(src, srcy + sx + 1)                           \
          + (r.x + tx) * getTexel_##FMT(src, srcy + sx + ofs2.x);              \
      const vec4f v1 = (h.x - tx) * getTexel_##FMT(src, srcy + srcSize.x + sx) \
          + h.x * getTexel_##FMT(src, srcy + srcSize.x + sx + 1)               \
          + (r.x + tx) * getTexel_##FMT(src, srcy + srcSize.x + sx + ofs2.x);  \
      const vec4f v2 = (h.x - tx) * getTexel_##FMT(src, ofs2.y + sx)           \
          + h.x * getTexel_##FMT(src, ofs2.y + sx + 1)                         \
          + (r.x + tx) * getTexel_##FMT(src, ofs2.y + sx + ofs2.x);            \
      val = wy0 * v0 + h.y * v1 + wy2 * v2;                                    \
      setTexel_##FMT(dst, dsty + x, val);                                      \
    }                                                                          \
    break;

task unmasked void MipMap_generateRows(const void *uniform const src,
    const uniform vec2i &srcSize,
    void *uniform const dst,
    const uniform vec2i &dstSize,
    const uniform uint32 format)
{
  const uniform int y = taskIndex;
  vec4f val;
  const uniform int dsty = y * dstSize.x;
  const uniform int srcy = 2 * y * srcSize.x;

  uniform vec2f r = rcp(make_vec2f(srcSize.x, srcSize.y));
  uniform vec2f h = make_vec2f(dstSize.x, dstSize.y) * r;
  uniform vec2i ofs2 = make_vec2i(2, srcy + 2 * srcSize.x);
  if ((srcSize.x & 1) == 0) {
    h.x = 0.5f;
    r.x = 0.f;
    ofs2.x = 0;
  }
  if ((srcSize.y & 1) == 0) {
    h.y = 0.5f;
    r.y = 0.f;
    ofs2.y = srcy;
  }

  const uniform float wy0 = h.y - y * r.y;
  const uniform float wy2 = r.y + y * r.y;

  switch (format) {
    __foreach_fetcher(__define_tex_filterkernel_case);
  default:
    break;
  }
}
#undef __define_tex_filterkernel_case

#define __define_tex_filterkernel_case1D(FMT)                                  \
  case OSP_TEXTURE_##FMT:                                                      \
    foreach (x = 0 ... dstSize) {                                              \
      const float tx = x * r;                                                  \
      const float sx = 2 * x;                                                  \
      val = (h - tx) * getTexel_##FMT(src, sx)                                 \
          + h * getTexel_##FMT(src, sx + 1)                                    \
          + (r + tx) * getTexel_##FMT(src, sx + ofs2);                         \
      setTexel_##FMT(dst, x, val);                                             \
    }                                                                          \
    break;

void MipMap_generateRow(const void *uniform const src,
    const uniform int &srcSize,
    void *uniform const dst,
    const uniform int &dstSize,
    const uniform uint32 format)
{
  vec4f val;
  uniform float r = rcp((float)srcSize);
  uniform float h = (float)dstSize * r;
  uniform int ofs2 = 2;
  if ((srcSize & 1) == 0) {
    h = 0.5f;
    r = 0.f;
    ofs2 = 0;
  }
  switch (format) {
    __foreach_fetcher(__define_tex_filterkernel_case1D);
  default:
    break;
  }
}
#undef __define_tex_filterkernel_case1D

export void MipMap_generate(const void *uniform const src,
    const uniform vec2i &srcSize,
    void *uniform const dst,
    const uniform vec2i &dstSize,
    const uniform uint32 format)
{
  if (srcSize.x > 1 && srcSize.y > 1) {
    launch[dstSize.y] MipMap_generateRows(src, srcSize, dst, dstSize, format);
    sync;
  } else if (srcSize.x > 1 || srcSize.y > 1) {
    const uniform int srcSize = max(srcSize.x, srcSize.y);
    const uniform int dstSize = max(dstSize.x, dstSize.y);
    MipMap_generateRow(src, srcSize, dst, dstSize, format);
  }
}
