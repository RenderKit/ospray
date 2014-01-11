/*! \file testrenderer.ispc Defines ISPC side of test frame renderer
    (that only renders a simple test frame */

// ospray stuff
#include "../fb/framebuffer.ih"
#include "../camera/perspectivecamera.ih"
#include "../common/ray.ih"
// embree stuff
#include "embree2/rtcore_scene.isph"

inline vec3f make_random_color(const int i)
{
  const int mx = 13*17*43;
  const int my = 11*29;
  const int mz = 7*23*63;
  const uint32 g = (i * (3*5*127)+12312314);
  return make_vec3f((g % mx)*(1.f/(mx-1)),
                    (g % my)*(1.f/(my-1)),
                    (g % mz)*(1.f/(mz-1)));
}

task void RayCastRenderer_renderLine(uniform FrameBuffer *uniform fb,
                                     uniform RTCScene scene,
                                     uniform Camera *uniform camera)
{
  const uniform int size_x = fb->size.x;
  const uniform int size_y = fb->size.y;
  const uniform int y = taskIndex;
  foreach (x = 0...size_x) {
    // const int ix = (x+frameID)%size_x;
    // const int iy = (y+frameID)%size_y;
    // const int iz = (x+y);
    // const float fx = ((float)ix)/size_x;
    // const float fy = ((float)iy)/size_y;
    // const float fz = ((float)iz)/(size_x+size_y);
    // const float fw = 1.f; //0.f;

    const float screen_u = (x+.5f)/size_x;
    const float screen_v = (y+.5f)/size_y;
    // const vec3f ray_org
    //   = camera->plane_org
    //   + screen_u * camera->plane_du
    //   + screen_v * camera->plane_dv;
    // const vec3f ray_dir = camera->dir;
    
    Ray ray;
    camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
    // setRay(ray,ray_org,ray_dir);
    // ISPC issue #703. Switch to 'nice' code once ISPC issue 703 is
    // fixed.
#if 0
    rtcIntersect(scene,(varying RTCRay&)ray);
#else
    rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
#endif
    
    vec4f col = make_vec4f(0.f);

#if 0
    if (ray.geomID >= 0) {
      col = make_vec4f(abs(normalize(ray.Ng)));
    }
#else
    col = make_vec4f(make_random_color(ray.primID));
#endif

    fb->setPixel(fb,make_vec2i(x,y),col); //make_vec4f(fx,fy,fz,fw));
  }
}


/*! \brief entry point for the ISPC-side render frame function for the
  test frame renderer 
  
  \detailed \note As a publicly exported function the frame buffer
  parameter for this function actually has to be typed 'void *'
  because ISPC cannot export any (non-trivial) ISPC-side classes 
  
  \internal the reason we pass 'void*' typed parameters here is that the current
  ISPC version cannot export the base classes for the pointers 
*/
export void ispc__RayCastRenderer_renderFrame(void *uniform _camera,
                                              uniform RTCScene scene,
                                              void *uniform _fb
                                              )
{
  uniform FrameBuffer *uniform fb = (uniform FrameBuffer *uniform)_fb;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;

  launch[fb->size.y] RayCastRenderer_renderLine(fb,scene,camera);
}

