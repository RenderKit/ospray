// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.ih"

// Low-level texel accessors
//////////////////////////////////////////////////////////////////////////////

#define BYTE2FLOAT (1.f / 255.f)
#define SHORT2FLOAT (1.f / 65535.f)

// TODO blocking

inline vec4f getTexel_RGBA8(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint32 c =
      ((const uint32 *uniform)self->data)[i.y * self->size.x + i.x];
  const uint32 r = c & 0xff;
  const uint32 g = (c >> 8) & 0xff;
  const uint32 b = (c >> 16) & 0xff;
  const uint32 a = c >> 24;
  return make_vec4f((float)r, (float)g, (float)b, (float)a) * BYTE2FLOAT;
}

inline vec4f getTexel_RGBA16(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint32 texelOfs = (i.y * self->size.x + i.x) * 2; // fetch as 2x 32bit
  uint32 c = ((const uint32 *uniform)self->data)[texelOfs];
  const uint32 r = c & 0xffff;
  const uint32 g = c >> 16;
  c = ((const uniform uint32 *uniform)self->data)[texelOfs + 1];
  const uint32 b = c & 0xffff;
  const uint32 a = c >> 16;
  return make_vec4f((float)r, (float)g, (float)b, (float)a) * SHORT2FLOAT;
}

inline vec4f getTexel_RGB8(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint8 *uniform texel = (const uint8 *uniform)self->data;
  const uint32 texelOfs = 3 * (i.y * self->size.x + i.x);
  const uint32 r = texel[texelOfs];
  const uint32 g = texel[texelOfs + 1];
  const uint32 b = texel[texelOfs + 2];
  return make_vec4f(make_vec3f((float)r, (float)g, (float)b) * BYTE2FLOAT, 1.f);
}

inline vec4f getTexel_RGB16(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint16 *uniform texel = (const uint16 *uniform)self->data;
  const uint32 texelOfs = 3 * (i.y * self->size.x + i.x);
  const uint32 r = texel[texelOfs];
  const uint32 g = texel[texelOfs + 1];
  const uint32 b = texel[texelOfs + 2];
  return make_vec4f(
      make_vec3f((float)r, (float)g, (float)b) * SHORT2FLOAT, 1.f);
}

inline vec4f getTexel_RA8(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint16 *uniform texel = (const uint16 *uniform)self->data;
  const uint32 t = texel[i.y * self->size.x + i.x];
  const uint32 c = t & 0xff;
  const uint32 a = t >> 8;
  return make_vec4f(c * BYTE2FLOAT, 0.f, 0.f, a * BYTE2FLOAT);
}

inline vec4f getTexel_RA16(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint32 *uniform texel = (const uint32 *uniform)self->data;
  const uint32 t = texel[i.y * self->size.x + i.x];
  const uint32 c = t & 0xffff;
  const uint32 a = t >> 16;
  return make_vec4f(c * SHORT2FLOAT, 0.f, 0.f, a * SHORT2FLOAT);
}

inline vec4f getTexel_R8(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint8 c =
      ((const uniform uint8 *uniform)self->data)[i.y * self->size.x + i.x];
  return make_vec4f(c * BYTE2FLOAT, 0.f, 0.f, 1.f);
}

inline vec4f getTexel_R16(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint16 c =
      ((const uniform uint16 *uniform)self->data)[i.y * self->size.x + i.x];
  return make_vec4f(c * SHORT2FLOAT, 0.f, 0.f, 1.f);
}

// luminance (basically gamma-corrected grayscale) with alpha
inline vec4f getTexel_LA8(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint16 *uniform texel = (const uint16 *uniform)self->data;
  const uint32 t = texel[i.y * self->size.x + i.x];
  const uint32 l = t & 0xff;
  const uint32 a = t >> 8;
  return make_vec4f(make_vec3f(srgb_to_linear(l * BYTE2FLOAT)), a * BYTE2FLOAT);
}

inline vec4f getTexel_LA16(const Texture2D *uniform self, const vec2i i)
{
  const vec4f t = getTexel_RA16(self, i);
  return make_vec4f(t.x, t.x, t.x, t.w);
}

// luminance (basically gamma-corrected grayscale)
inline vec4f getTexel_L8(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  const uint8 l =
      ((const uniform uint8 *uniform)self->data)[i.y * self->size.x + i.x];
  return make_vec4f(make_vec3f(srgb_to_linear(l * BYTE2FLOAT)), 1.f);
}

inline vec4f getTexel_SRGBA(const Texture2D *uniform self, const vec2i i)
{
  return srgba_to_linear(getTexel_RGBA8(self, i));
}

inline vec4f getTexel_SRGB(const Texture2D *uniform self, const vec2i i)
{
  return srgba_to_linear(getTexel_RGB8(self, i));
}

inline vec4f getTexel_RGBA32F(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  return ((const uniform vec4f *uniform)self->data)[i.y * self->size.x + i.x];
}

inline vec4f getTexel_RGB32F(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  vec3f v =
      ((const uniform vec3f *uniform)self->data)[i.y * self->size.x + i.x];
  return make_vec4f(v, 1.f);
}

inline vec4f getTexel_R32F(const Texture2D *uniform self, const vec2i i)
{
  assert(self);
  float v =
      ((const uniform float *uniform)self->data)[i.y * self->size.x + i.x];
  return make_vec4f(v, 0.f, 0.f, 1.f);
}

// Texture coordinate utilities
//////////////////////////////////////////////////////////////////////////////

inline vec2i nearest_coords(const Texture2D *uniform self, const vec2f p)
{
  // repeat: get remainder within [0..1] parameter space
  vec2f tc = frac(p);
  tc = max(tc, make_vec2f(0.0f)); // filter out inf/NaN

  // scale by texture size
  tc = tc * self->sizef;

  // nearest
  return make_vec2i(tc);
}

struct BilinCoords
{
  vec2i st0;
  vec2i st1;
  vec2f frac;
};

inline BilinCoords bilinear_coords(const Texture2D * self, const vec2f &p)
{
  BilinCoords coords;

  // repeat: get remainder within [0..1] parameter space
  // lower sample shifted by half a texel
  vec2f tc = frac(p - self->halfTexel);
  tc = max(tc, make_vec2f(0.0f)); // filter out inf/NaN

  // scale by texture size
  tc = tc * self->sizef;
  coords.frac = frac(tc);

  coords.st0 = make_vec2i(tc);
  coords.st1 = coords.st0 + 1;
  // handle border cases
  if (coords.st1.x >= self->size.x)
    coords.st1.x = 0;
  if (coords.st1.y >= self->size.y)
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

// Implementations of Texture2D_get for different formats and filter modi
//////////////////////////////////////////////////////////////////////////////

#define __define_tex_get(FMT)                                                  \
                                                                               \
  static vec4f Texture2D_nearest_##FMT(                                        \
      const Texture2D *uniform self, const DifferentialGeometry &dg)           \
  {                                                                            \
    return getTexel_##FMT(self, nearest_coords(self, dg.st));                  \
  }                                                                            \
                                                                               \
  static vec4f Texture2D_bilinear_##FMT(                                       \
      const Texture2D *uniform self, const DifferentialGeometry &dg)           \
  {                                                                            \
    BilinCoords cs = bilinear_coords(self, dg.st);                             \
                                                                               \
    const vec4f c00 = getTexel_##FMT(self, make_vec2i(cs.st0.x, cs.st0.y));    \
    const vec4f c01 = getTexel_##FMT(self, make_vec2i(cs.st1.x, cs.st0.y));    \
    const vec4f c10 = getTexel_##FMT(self, make_vec2i(cs.st0.x, cs.st1.y));    \
    const vec4f c11 = getTexel_##FMT(self, make_vec2i(cs.st1.x, cs.st1.y));    \
                                                                               \
    return bilerp(cs.frac, c00, c01, c10, c11);                                \
  }                                                                            \
  struct swallow_semicolon

#define __define_tex_case(NAME, FMT)                                           \
  case OSP_TEXTURE_##FMT:                                                      \
    return filter_nearest ? &NAME##_nearest_##FMT : &NAME##_bilinear_##FMT

#define __define_tex_get_case(FMT) __define_tex_case(Texture2D, FMT)
#define __define_tex_getN_case(FMT) __define_tex_case(Texture2D_N, FMT)

#define __foreach_fetcher(FCT)                                                 \
  FCT(RGBA8);                                                                  \
  FCT(SRGBA);                                                                  \
  FCT(RGBA32F);                                                                \
  FCT(RGB8);                                                                   \
  FCT(SRGB);                                                                   \
  FCT(LA8);                                                                    \
  FCT(RA8);                                                                    \
  FCT(RGB32F);                                                                 \
  FCT(R8);                                                                     \
  FCT(L8);                                                                     \
  FCT(R32F);                                                                   \
  FCT(RGBA16);                                                                 \
  FCT(RGB16);                                                                  \
  FCT(RA16);                                                                   \
  FCT(R16)

__foreach_fetcher(__define_tex_get);

static uniform Texture2D_get Texture2D_get_addr(
    const uniform uint32 type, const uniform bool filter_nearest)
{
  switch (type) {
    __foreach_fetcher(__define_tex_get_case);
  }
  return NULL;
};

static vec3f Texture2D_Normal_neutral(
    const Texture2D *uniform self, const DifferentialGeometry &)
{
  return make_vec3f(0.f, 0.f, 1.f);
}

#define __define_tex_getN_flt(NAME, C)                                         \
  static vec3f Texture2D_N_##NAME(                                             \
      const Texture2D *uniform self, const DifferentialGeometry &dg)           \
  {                                                                            \
    return make_vec3f(Texture2D_##NAME(self, dg)) * C - 1.f;                   \
  }                                                                            \
  struct swallow_semicolon

#define __define_tex_getN(FMT, C)                                              \
  __define_tex_getN_flt(nearest_##FMT, C);                                     \
  __define_tex_getN_flt(bilinear_##FMT, C)

__define_tex_getN(RGB8, (255.f / 127.f));
__define_tex_getN(RGBA8, (255.f / 127.f));
__define_tex_getN(RGB32F, 2.f);
__define_tex_getN(RGBA32F, 2.f);
__define_tex_getN(RGB16, (65535.f / 32767.f));
__define_tex_getN(RGBA16, (65535.f / 32767.f));

static uniform Texture2D_getN Texture2D_getN_addr(
    const uniform uint32 type, const uniform bool filter_nearest)
{
  switch (type) {
  case OSP_TEXTURE_SRGBA: /* fallthrough, sRGB ignored for normals */
    __define_tex_getN_case(RGBA8);
  case OSP_TEXTURE_SRGB: /* fallthrough, sRGB ignored for normals */
    __define_tex_getN_case(RGB8);
    __define_tex_getN_case(RGBA32F);
    __define_tex_getN_case(RGB32F);
    __define_tex_getN_case(RGBA16);
    __define_tex_getN_case(RGB16);
  }
  return &Texture2D_Normal_neutral;
};

#undef __define_tex_get
#undef __define_tex_getN
#undef __define_tex_getN_flt
#undef __define_tex_get_addr
#undef __define_tex_case
#undef __define_tex_get_case
#undef __define_tex_getN_case
#undef __foreach_fetcher

#undef BYTE2FLOAT
#undef SHORT2FLOAT

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void *uniform Texture2D_create(uniform vec2i &size,
    void *uniform data,
    uniform uint32 type,
    uniform uint32 flags,
    void *uniform map1d
    )
{
  Texture2D *uniform self = uniform new Texture2D;
  self->size = size;

  // Due to float rounding frac(x) can be exactly 1.0f (e.g. for very small
  // negative x), although it should be strictly smaller than 1.0f. We handle
  // this case by having sizef slightly smaller than size, such that
  // frac(x)*sizef is always < size.
  self->sizef = make_vec2f(
      nextafter((float)size.x, -1.0f), nextafter((float)size.y, -1.0f));
  self->halfTexel = make_vec2f(0.5f / size.x, 0.5f / size.y);
  self->data = data;
  self->map1d = (uniform EnsightTex1dMappingData *)map1d;
  self->get = Texture2D_get_addr(type, flags & OSP_TEXTURE_FILTER_NEAREST);
  self->getNormal =
      Texture2D_getN_addr(type, flags & OSP_TEXTURE_FILTER_NEAREST);
  self->hasAlpha = type == OSP_TEXTURE_RGBA8 || type == OSP_TEXTURE_SRGBA
      || type == OSP_TEXTURE_RA8 || type == OSP_TEXTURE_LA8
      || type == OSP_TEXTURE_RGBA32F || type == OSP_TEXTURE_RGBA16
      || type == OSP_TEXTURE_RA16;
  return self;
}

inline vec3f sampleTexture1D(const Texture2D *tex, float t, bool isbanded) {
    const vec3f *pal = (const vec3f *)tex->data;
    const int size1 = tex->size.x - 1;
    vec3f out;
    if (t <= 0.0f) {
        out = pal[0];
    }
    else {
        const float indexf = t * tex->sizef.x;     
        if (indexf >= size1) {
            out = pal[size1];
        }
        else{
            const float tmp = floor(indexf);
            const int index = (int)tmp;
            if (isbanded) {
                out = pal[index];
            }
            else {
                const float dt = indexf - tmp;
                out = pal[index] * (1.0f - dt) + pal[index + 1] * dt;
            }
        }
    }
    return out;
};

vec3f sampleTexture1DByRawVar(const Texture2D *tex, float t)
{
    const EnsightTex1dMappingOut out = EnsightTex1dMapping_getT(tex->map1d, t);
    vec3f ret;
    if (out.usergba){
        ret.x = out.rgba[0], ret.y = out.rgba[1], ret.z = out.rgba[2];
    }
    else {
        ret = sampleTexture1D(tex, out.t, out.isbanded);
    }
    return ret;
}

float sampleAlphaTexture1DByRawVar(const Texture2D *tex, float t)
{
    if (NULL == tex) {
        return 1.0f;
    }
    const EnsightTex1dMappingOut out = EnsightTex1dMapping_getT(tex->map1d, t);
    if (out.usergba){
        return 1.0f;
    }
    const vec3f ret = sampleTexture1D(tex, out.t, false);    
    return ret.x;
}

inline vec4f getTexel(const Texture2D * self, const vec2i i) {
  vec4f ret;
  const vec3f v = ((const vec3f *)self->data)[i.y * self->size.x + i.x];
  ret = make_vec4f(v, 1.0f);
  return ret;
}

const float BLACKFLAG = -123456; //special value which should never appear, since return value range should be [0..1]

inline BilinCoords bilinear_coords2(const Texture2D * self, const vec2f &uv, int imgwrap) {
  BilinCoords coords;
  vec2f p = uv;
  if (TEXTURE_REPEAT == imgwrap){
    p = frac(p);
  }
  else if (TEXTURE_BLACK == imgwrap) {
    if ((p.x < 0 || p.x > 1) || (p.y < 0 || p.y > 1)){
        coords.frac.x = BLACKFLAG;
        return coords;
    }
  }
  else { //(TEXTURE_CLAMP == imgwrap || TEXTURE_CLAMP_TO_TEXTURE == imgwrap)
    p.x = clamp(p.x, 0, 1);
    p.y = clamp(p.y, 0, 1);
  }

  vec2f p1 = max(p, self->halfTexel);
  vec2f tc = frac(p1 - self->halfTexel);

  // scale by texture size
  tc = tc * self->sizef;
  coords.frac = frac(tc);

  coords.st0 = make_vec2i(tc);
  coords.st1 = coords.st0 + 1;
  // handle border cases
  if (coords.st1.x >= self->size.x)
    coords.st1.x = self->size.x - 1;
  if (coords.st1.y >= self->size.y)
    coords.st1.y = self->size.y - 1;

  return coords;
}

inline vec4f texture2d(const Texture2D * self, const DifferentialGeometry &dg, int imgwrap) {                                                                            
    const BilinCoords cs = bilinear_coords2(self, dg.st, imgwrap);
    if (BLACKFLAG == cs.frac.x) {
        return make_vec4f(0, 0, 0, 1);
    }
    const vec4f c00 = getTexel(self, make_vec2i(cs.st0.x, cs.st0.y));    
    const vec4f c01 = getTexel(self, make_vec2i(cs.st1.x, cs.st0.y));    
    const vec4f c10 = getTexel(self, make_vec2i(cs.st0.x, cs.st1.y));    
    const vec4f c11 = getTexel(self, make_vec2i(cs.st1.x, cs.st1.y));                                                                                   
    return bilerp(cs.frac, c00, c01, c10, c11);                                
}                                                                            

inline float get_y(const vec3f &c){
    const float YWeight[3] = { 0.212671f, 0.715160f, 0.072169f };
    return YWeight[0] * c.x + YWeight[1] * c.y + YWeight[2] * c.z;
}

vec3f sampleTexture2DEnsight(const Texture2D *tex, int imgwrap, int texmode, const DifferentialGeometry &dg, const vec3f &le)
{
    vec3f L = le;
    const vec4f pixel = texture2d(tex, dg, imgwrap);
    const vec3f k = make_vec3f(pixel.x, pixel.y, pixel.z);
    if (TEXTURE_MODE_MODULATE == texmode){
        L = L * k;
    }
    else if (TEXTURE_MODE_DECAL == texmode){
        const float t = pixel.w;
        L = (1.0f - t) * L + (t * get_y(L)) * k;
    }
    else if (TEXTURE_MODE_REPLACE == texmode){
        //we have to consider whether point is under shadow or not, but we don't want 
        //to sample the texture multiple times for each light source. So we use the luminance 
        //of the computed result instead to approximate
        L = get_y(L) * k;
    }
    return L;
}


