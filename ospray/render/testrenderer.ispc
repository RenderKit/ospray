/*! \file testrenderer.ispc Defines ISPC side of test frame renderer
    (that only renders a simple test frame */

#include "../fb/framebuffer.ih"
#include "../common/math/vec.ih"

task void TestRenderer_renderLine(uniform FrameBuffer *uniform fb,
                                  uniform int size_x,
                                  uniform int size_y,
                                  uniform int frameID)
{
  const uniform int y = taskIndex;
  foreach (x = 0 ... size_x) {
    const int ix = (x+frameID)%size_x;
    const int iy = (y+frameID)%size_y;
    const int iz = (x+y);
    const float fx = ((float)ix)/size_x;
    const float fy = ((float)iy)/size_y;
    const float fz = ((float)iz)/(size_x+size_y);
    const float fw = 1.f; //0.f;
    fb->setPixel(fb,make_vec2i(x,y),make_vec4f(fx,fy,fz,fw));
  }
}


/*! \brief entry point for the ISPC-side render frame function for the
  test frame renderer */
/* \detailed \note As a publicly exported function the frame buffer
    parameter for this function actually has to be typed 'void *'
    because ISPC cannot export any (non-trivial) ISPC-side classes */
export void ispc__TestRenderer_renderFrame(void *uniform _fb,
                                           uniform int frameID)
{
  uniform FrameBuffer *uniform fb = (uniform FrameBuffer *uniform)_fb;
  launch[fb->size.y] TestRenderer_renderLine(fb,fb->size.x,fb->size.y,frameID);
}

