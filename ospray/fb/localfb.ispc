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

export void *uniform ispc__createLocalFB_RGBA_F32(const uniform uint size_x,
                                                  const uniform uint size_y,
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
void localfb_uint32_destruct(uniform FrameBuffer *uniform fb)
{
  delete fb;
}
void localfb_uint32_setPixel(uniform FrameBuffer *uniform _fb,
                            const varying vec2i &coord,
                            const varying uint32 &value)
{
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  uniform uint32 *uniform pixel = (uniform uint32 *uniform)fb->pixel;
  pixel[coord.x+coord.y*fb->inherited.size.x] = value;
}

inline float extractUcharChannel(unsigned int pix, uniform const int shift)
{
  unsigned int i = (pix >> shift) & 255;
  return i * (1.f/255.f);
}

uint32 localfb_uint32_getPixel(uniform FrameBuffer *uniform _fb,
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

export void *uniform ispc__createLocalFB_RGBA_I8(const uniform uint size_x,
                                                 const uniform uint size_y,
                                                 void *uniform cClassPtr,
                                                 void *uniform pixelPtr)
{
  uniform LocalFB *uniform fb = uniform new uniform LocalFB;
  fb->inherited.destructor = localfb_uint32_destruct;
  fb->inherited.setPixel   = localfb_uint32_setPixel;
  fb->inherited.getPixel   = localfb_uint32_getPixel;
  fb->inherited.cClassPtr  = cClassPtr;
  fb->inherited.size.x     = size_x;
  fb->inherited.size.y     = size_y;
  fb->pixel = pixelPtr;
}


