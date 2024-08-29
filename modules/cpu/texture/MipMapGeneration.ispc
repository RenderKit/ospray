// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2DFormats.ih"

task unmasked void MipMap_generateRows(const void *uniform const src,
    const uniform vec2i &srcSize,
    void *uniform const dst,
    const uniform vec2i &dstSize,
    const uniform OSPTextureFormat format)
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

  foreach (x = 0 ... dstSize.x) {
    const float tx = x * r.x;
    const int sx = 2 * x;
    const vec4f v0 = (h.x - tx) * Texture2D_getTexel(format, src, srcy + sx)
        + h.x * Texture2D_getTexel(format, src, srcy + sx + 1)
        + (r.x + tx) * Texture2D_getTexel(format, src, srcy + sx + ofs2.x);
    const vec4f v1 =
        (h.x - tx) * Texture2D_getTexel(format, src, srcy + srcSize.x + sx)
        + h.x * Texture2D_getTexel(format, src, srcy + srcSize.x + sx + 1)
        + (r.x + tx)
            * Texture2D_getTexel(format, src, srcy + srcSize.x + sx + ofs2.x);
    const vec4f v2 = (h.x - tx) * Texture2D_getTexel(format, src, ofs2.y + sx)
        + h.x * Texture2D_getTexel(format, src, ofs2.y + sx + 1)
        + (r.x + tx) * Texture2D_getTexel(format, src, ofs2.y + sx + ofs2.x);
    val = wy0 * v0 + h.y * v1 + wy2 * v2;
    Texture2D_setTexel(format, dst, dsty + x, val);
  }
}

void MipMap_generateRow(const void *uniform const src,
    const uniform int &srcSize,
    void *uniform const dst,
    const uniform int &dstSize,
    const uniform OSPTextureFormat format)
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

  foreach (x = 0 ... dstSize) {
    const float tx = x * r;
    const float sx = 2 * x;
    val = (h - tx) * Texture2D_getTexel(format, src, sx)
        + h * Texture2D_getTexel(format, src, sx + 1)
        + (r + tx) * Texture2D_getTexel(format, src, sx + ofs2);
    Texture2D_setTexel(format, dst, x, val);
  }
}

export void MipMap_generate(const void *uniform const src,
    const uniform vec2i &srcSize,
    void *uniform const dst,
    const uniform vec2i &dstSize,
    const uniform uint32 format)
{
  if (srcSize.x > 1 && srcSize.y > 1) {
    launch[dstSize.y] MipMap_generateRows(
        src, srcSize, dst, dstSize, (const uniform OSPTextureFormat)format);
    sync;
  } else if (srcSize.x > 1 || srcSize.y > 1) {
    const uniform int srcSize = max(srcSize.x, srcSize.y);
    const uniform int dstSize = max(dstSize.x, dstSize.y);
    MipMap_generateRow(
        src, srcSize, dst, dstSize, (const uniform OSPTextureFormat)format);
  }
}
