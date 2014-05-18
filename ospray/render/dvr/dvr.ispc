/*! \file dvr.ispc Defines ISPC side of "Direct Volume (ray casting) Renderer" (DVR) */

// ospray stuff
#include "ospray/fb/framebuffer.ih"
#include "ospray/camera/perspectivecamera.ih"
#include "ospray/common/ray.ih"
#include "ospray/math/bbox.ih"
#include "ospray/volume/volume.ih"


//! performs a ray-box intersection test, and returns intersection interval [t0,t1]
inline void boxtest(const Ray &ray,
                    const vec3f &rdir,
                    const uniform box3f &box,
                    float& t0,
                    float& t1)
{
  const vec3f mins = rdir * (box.lower - ray.org); //mul(sub(box.lower, ray.org), rdir);
  const vec3f maxs = rdir * (box.upper - ray.org); //mul(sub(box.upper, ray.org), rdir);
  
  t0 = max(max(ray.t0, 
               min(mins.x,maxs.x)),
           max(min(mins.y,maxs.y),
               min(mins.z,maxs.z)));
  
  t1 = min(min(ray.t, 
               max(mins.x,maxs.x)),
           min(max(mins.y,maxs.y),
               max(mins.z,maxs.z)));
}



//! do direct volume rendering via ray casting 
/*! \internal this is code adapted from some earlier code that was mostly written by aaron knoll */
inline vec4f rayCastDVR(const Ray &ray, 
                        uniform Volume *uniform volume)
{
  const uniform box3f box = make_box3f(make_vec3f(0.f),volume->clampSize);
  const vec3f rdir = rcp(ray.dir);
  
  vec4f color = make_vec4f(0.f);

  float tenter, texit;
  boxtest(ray, rdir, box, tenter, texit);
  if (tenter >= texit) return color;

  float step_size = .5;

#define QUADRATIC_SAMPLING 0
#if QUADRATIC_SAMPLING
  float ray_diff = 1.0;
  float rda = ray.rda * ray_diff;
  float rdb = sqrt(rda);
  float inv_rda = 1.0 / rda;
#endif

  if (tenter < texit)
  {
    vec3f p0; //, p1; 
    float t0, t1;
    float f0 = 0.0;
    // float f1;

    t1 = tenter;

    //AARON NOTE: we will want some sort of space skipping here; or to call dvr() as an intersection routine on accel nodes.

#if QUADRATIC_SAMPLING
    //find first sample in quadratic sampling pattern
    float ceil_x = max(floor((sqrt(rdb * rdb + 2.0 * rda * tenter) - rdb) * inv_rda), 0.0);
    t1 = max(0.5*rda*ceil_x*ceil_x + rdb*ceil_x - (1.0 / 65536.0), t1);

    float dt = sqrt(rdb*rdb + 2.0 * rda * t1);
    float M = 1.0;          //sample rate multiplier, e.g. per macrocell
    float frda = M*(M + 1.0)*.5*rda;
    step_size = M*dt + frda;
#else
    t1 = max(floor(tenter / step_size) * step_size, t1);
#endif

    // p1 = ray.org + t1 * ray.dir;

    while(t1 < texit && color.w < .97f)
    {
      // p0 = p1;
      t1 += step_size;
#if QUADRATIC_SAMPLING
      step_size += frda;
#endif

      const vec3f p1 = ray.org + t1 * ray.dir;
      const float f1 = volume->lerpf(volume, p1);

      vec4f color_sample = make_vec4f(f1); //f1 * make_vec4f(1); //, f1);
      
      const float alpha = 1.0 - exp(-color_sample.w * step_size);
      const float alpha_1msa = alpha * (1.0-color.w);
    
      color_sample.w = 1.f;
      color = color + color_sample * alpha_1msa;
    }

    // ray.color = color;
  }
  // else
  // {
  //   ray.color = make_vec4f(0,0,0,1);
  // } 
  
  return color;
  // return make_vec4f(abs(ray.dir));
}

//! render a line with DVR rendering; 
/*! \internal this should eventually get moved to a tile-based
  renderer (ideally with load balancer support for distributed
  rendering), but right now lines is easier */
task void DVRRenderer_renderLine(uniform FrameBuffer *uniform fb,
                                 uniform Volume *uniform volume,
                                 uniform Camera *uniform camera)
{
  const uniform int size_x = fb->size.x;
  const uniform int size_y = fb->size.y;
  const uniform int y = taskIndex;
  foreach (x = 0 ... size_x) {
    const float screen_u = (x+.5f)/size_x;
    const float screen_v = (y+.5f)/size_y;
    
    Ray ray;
    camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));

    const vec4f col = rayCastDVR(ray,volume);
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
export void ispc__DVRRenderer_renderFrame(void *uniform _camera,
                                          void *uniform _volume,
                                          void *uniform _fb
                                          )
{
  uniform FrameBuffer *uniform fb = (uniform FrameBuffer *uniform)_fb;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
  
  launch[fb->size.y] DVRRenderer_renderLine(fb,volume,camera);
}

