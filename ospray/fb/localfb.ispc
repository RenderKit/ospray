/*! \file localfb.ispc ISPC-side code for a local frame buffer */

#include "framebuffer.ih"

struct LocalFB 
{
  FrameBuffer inherited; /*!< inherit all methods and members from 'base' class */
  void *pixel;
};

// =======================================================
// float4 version of frame buffer 
// =======================================================
void localfb_vec4f_destruct(uniform FrameBuffer *uniform fb)
{
  delete fb;
}
void localfb_vec4f_setPixel(uniform FrameBuffer *uniform _fb,
                            const varying vec2i &coord,
                            const varying vec4f &value)
{
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  uniform vec4f *uniform pixel = (uniform vec4f *uniform)fb->pixel;
  pixel[coord.x+coord.y*fb->inherited.size.x] = value;
}

vec4f localfb_vec4f_getPixel(uniform FrameBuffer *uniform _fb,
                             const varying vec2i &coord)
{
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  uniform vec4f *uniform pixel = (uniform vec4f *uniform)fb->pixel;
  return pixel[coord.x+coord.y*fb->inherited.size.x];
}

export void *uniform ispc__createLocalFB_RGBA_F32(const uniform uint32 size_x,
                                                  const uniform uint32 size_y,
                                                  void *uniform cClassPtr,
                                                  void *uniform pixelPtr)
{
  uniform LocalFB *uniform fb = uniform new uniform LocalFB;
  fb->inherited.destructor = localfb_vec4f_destruct;
  fb->inherited.setPixel   = localfb_vec4f_setPixel;
  fb->inherited.getPixel   = localfb_vec4f_getPixel;
  fb->inherited.cClassPtr  = cClassPtr;
  fb->inherited.size.x     = size_x;
  fb->inherited.size.y     = size_y;
  fb->pixel = pixelPtr;
}

// =======================================================
// vec4uc version of frame buffer 
// =======================================================
inline float extractUcharChannel(uint32 pix, uniform const int32 shift)
{
  uint32 i = (pix >> shift) & 255;
  return i * (1.f/255.f);
}

inline uint32 cvt_uint32(const float f)
{
  return (uint32)(255.f * max(min(f,1.f),0.f));
}

inline uint32 cvt_uint32(const vec4f &v)
{
  return 
    (cvt_uint32(v.x) << 0)  |
    (cvt_uint32(v.y) << 8)  |
    (cvt_uint32(v.z) << 16) |
    (cvt_uint32(v.w) << 24);
}

void localfb_uint32_destruct(uniform FrameBuffer *uniform fb)
{
  delete fb;
}

void localfb_uint32_setPixel(uniform FrameBuffer *uniform _fb,
                            const varying vec2i &coord,
                            const varying vec4f &value)
{
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  uniform uint32 *uniform pixel = (uniform uint32 *uniform)fb->pixel;

  const uint32 ofs = coord.x+coord.y*fb->inherited.size.x;
  pixel[ofs] = cvt_uint32(value);
}

void localfb_uint32_setTile(uniform FrameBuffer *uniform _fb,
                            uniform Tile *uniform tile)
{
#if 1
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  uniform uint32 *uniform pixel = (uniform uint32 *uniform)fb->pixel;
  for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32  pixID = i*programCount+programIndex;
    const uint32  x     = tile->region.lower.x + (pixID % TILE_SIZE);
    const uint32  y     = tile->region.lower.y + (pixID / TILE_SIZE);
    const uint32  ofs   = y*tile->fbSize.x+x;
    const vec4f value = make_vec4f(tile->r[i],tile->g[i],tile->b[i],tile->a[i]);
    if (x < tile->fbSize.x & y < tile->fbSize.y) {
      pixel[ofs]        = cvt_uint32(value);
    }
  }
#endif
}

vec4f localfb_uint32_getPixel(uniform FrameBuffer *uniform _fb,
                              const varying vec2i &coord)
{
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  uniform uint32 *uniform pixel = (uniform uint32 *uniform)fb->pixel;
  uint32 pix = pixel[coord.x+coord.y*fb->inherited.size.x];
  return make_vec4f(extractUcharChannel(pix,0),
                    extractUcharChannel(pix,8),
                    extractUcharChannel(pix,16),
                    extractUcharChannel(pix,24));
}

export void *uniform ispc__createLocalFB_RGBA_I8(const uniform uint32 size_x,
                                                 const uniform uint32 size_y,
                                                 void *uniform cClassPtr,
                                                 void *uniform pixelPtr)
{
  uniform LocalFB *uniform fb = uniform new uniform LocalFB;
  fb->inherited.destructor = localfb_uint32_destruct;
  fb->inherited.setPixel   = localfb_uint32_setPixel;
  fb->inherited.getPixel   = localfb_uint32_getPixel;
  fb->inherited.setTile    = localfb_uint32_setTile;
  fb->inherited.cClassPtr  = cClassPtr;
  fb->inherited.size.x     = size_x;
  fb->inherited.size.y     = size_y;
  fb->pixel = pixelPtr;
}

export void ispc__setTile(void *uniform _fb, void *uniform _tile)
{
  uniform Tile *uniform        tile = (uniform Tile *uniform)_tile;
  uniform FrameBuffer *uniform fb   = (uniform FrameBuffer *uniform)_fb;
  fb->setTile(fb,tile);
}

