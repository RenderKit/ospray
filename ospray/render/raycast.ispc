/*! \file testrenderer.ispc Defines ISPC side of test frame renderer
    (that only renders a simple test frame */

// ospray stuff
#include "../fb/framebuffer.ih"
#include "../camera/perspectivecamera.ih"
#include "../common/ray.ih"
#include "util.ih"

// embree stuff
#include "embree2/rtcore_scene.isph"

// #define SHADE(ray,scene) shade_primID(ray)
// #define SHADE(ray,scene) shade_Ng(ray)
#define SHADE(ray,scene) shade_eyeLight(ray)
// #define SHADE(ray,scene) shade_eyeLight_testShadow(ray,scene)

inline vec4f shade_Ng(const Ray &ray)
{ 
  if (ray.geomID < 0) return make_vec4f(0.f); 
  return make_vec4f(abs(normalize(ray.Ng))); 
}

inline vec4f shade_primID(const Ray &ray)
{ 
  return make_vec4f(make_random_color(ray.primID)); 
}

inline vec4f shade_eyeLight(const Ray &ray)
{ 
  if (ray.geomID < 0) return make_vec4f(0.f); 
  const float c = dot(normalize(ray.Ng),ray.dir); 
  return make_vec4f(.3f+.8f*abs(c));
}

inline vec4f shade_eyeLight_testShadow(const Ray &ray, uniform RTCScene scene)
{ 
  if (ray.geomID < 0) return make_vec4f(0.f); 
  float c = dot(normalize(ray.Ng),ray.dir); 

  Ray shadowRay;
  const vec3f P = ray.org + (ray.t *.999f) * ray.dir;
  const vec3f L = make_vec3f(-1,2,-.5);
  setRay(shadowRay,P,L);
  rtcOccluded(scene,*((varying RTCRay *uniform)&shadowRay));
  const bool occluded = shadowRay.geomID >= 0;
  return make_vec4f(occluded 
                    ? .2f+.2f*abs(c)
                    : .3f+.8f*abs(c));
}

//! render a line with ray casting; 
/*! \internal this should eventually get moved to a tile-based
  renderer (ideally with load balancer support for distributed
  rendering), but right now lines is easier */
task void RayCastRenderer_renderLine(uniform FrameBuffer *uniform fb,
                                     uniform RTCScene scene,
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
    // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
#if 0
    rtcIntersect(scene,(varying RTCRay&)ray);
#else
    rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
#endif
    
    const vec4f col = SHADE(ray,scene);
    fb->setPixel(fb,make_vec2i(x,y),col);
  }
}



export void ispc__RayCastRenderer_renderTile(void *uniform _tile, //uniform Tile *uniform tile,
                                             void *uniform _camera, //uniform Camera *uniform camera,
                                             uniform RTCScene scene)
{
  uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;
  tile->format = TILE_FORMAT_RGBA8 | TILE_FORMAT_FLOAT4;
  const uniform uint32 size_x = tile->fbSize.x;
  const uniform uint32 size_y = tile->fbSize.y;
  const uniform uint32 x0 = tile->region.lower.x;
  const uniform uint32 y0 = tile->region.lower.y;
  for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32 frag = i*programCount+programIndex;
    const uint32  x     = x0 + (frag % TILE_SIZE);
    const uint32  y     = y0 + (frag / TILE_SIZE);
    // print("pixel % %\n",x,y);
    if (x < size_x & y < size_y) {
      // print("+pixel % %\n",x,y);
      const float screen_u = (x+.5f)/size_x;
      const float screen_v = (y+.5f)/size_y;
      Ray ray;
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
      // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
      // print("ray.dir % % %\n",ray.dir.x,ray.dir.y,ray.dir.z);
#if 0
      rtcIntersect(scene,(varying RTCRay&)ray);
#else
      rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
#endif
      const vec4f col = SHADE(ray,scene);
      // if (ray.geomID >= 0)
      //   print("col % % %\n",col.x,col.y,col.z);
#if 1
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.t;
      tile->rgba8[i] = cvt_uint32(col);
#endif
    }
  }
}

// extern "C" void ispc__RayCastRenderer_renderTile(void *uniform _tile, 
//                                              void *uniform _camera, 
//                                              uniform RTCScene scene
//                                              )
// {
//   uniform Camera *uniform camera = (uniform Camera *uniform)_camera;
//   uniform Tile *uniform tile = (uniform Tile *uniform)_tile;
//   launch[(TILE_SIZE/QUAD_SIZE)*(TILE_SIZE/QUAD_SIZE)] 
//     RayCastRenderer_renderQuad(tile,camera,scene);
// }



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
  // uniform FrameBuffer *uniform fb = (uniform FrameBuffer *uniform)_fb;
  // uniform Camera *uniform camera = (uniform Camera *uniform)_camera;

  // launch[fb->size.y] RayCastRenderer_renderLine(fb,scene,camera);
}

