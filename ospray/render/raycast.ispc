/*! \file testrenderer.ispc Defines ISPC side of test frame renderer
    (that only renders a simple test frame */

// ospray stuff
#include "../fb/framebuffer.ih"
#include "../camera/perspectivecamera.ih"
#include "../common/ray.ih"
#include "util.ih"

// embree stuff
#include "embree2/rtcore_scene.isph"

<<<<<<< HEAD
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
=======
inline vec3f make_random_color(const int i)
{
  const int mx = 13*17*43;
  const int my = 11*29;
  const int mz = 7*23*63;
  const uint32 g = (i * (3*5*127)+12312314);
  return make_vec3f((g % mx)*(1.f/(mx-1)),
                    (g % my)*(1.f/(my-1)),
                    (g % mz)*(1.f/(mz-1)));
>>>>>>> e6230d4664312113b687abcc9af63feda416b053
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
  foreach (x = 0...size_x) {
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
// extern "C" 
// void ispc__RayCastRenderer_renderTile(uniform Tile *uniform tile,
//                                       uniform Camera *uniform _camera,
//                                       uniform RTCScene scene)
{
  uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;

  const uniform uint size_x = tile->fbSize.x;
  const uniform uint size_y = tile->fbSize.y;
  // const uniform uint quadID = taskIndex;
  // const uniform uint quad_x = (quadID % (TILE_SIZE/QUAD_SIZE));
  // const uniform uint quad_y = (quadID / (TILE_SIZE/QUAD_SIZE));
  const uniform uint x0 = tile->region.lower.x;
  const uniform uint y0 = tile->region.lower.y;
  //  print("tile % %\n",x0,y0);
  for (uniform uint i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
  // foreach (frag=0 ... TILE_SIZE*TILE_SIZE/programCount) {
    const uint frag = i*programCount+programIndex;
    const uint  x     = x0 + (frag % TILE_SIZE);
    const uint  y     = y0 + (frag / TILE_SIZE);
    // print("frag % x y % %\n",frag,x,y);
    if (x < size_x & y < size_y) {
      const float screen_u = (x+.5f)/size_x;
      const float screen_v = (y+.5f)/size_y;
      Ray ray;
      // print("ray % % \n",screen_u,screen_v);
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
      // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
#if 0
      rtcIntersect(scene,(varying RTCRay&)ray);
#else
      rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
#endif
      
      const vec4f col = SHADE(ray,scene);
#if 1
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.t;
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

