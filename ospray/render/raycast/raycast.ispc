/*! \file raycast.ispc Defines ISPC side of the ray cast renderer */

// ospray stuff
#include "../../fb/framebuffer.ih"
#include "../../camera/perspectivecamera.ih"
#include "ospray/common/model.ih"
#include "../util.ih"

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

inline vec4f shade_geomID(const Ray &ray)
{ 
  return make_vec4f(make_random_color(ray.geomID)); 
}

inline vec4f shade_instID(const Ray &ray)
{ 
  return make_vec4f(make_random_color(ray.instID)); 
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

inline vec4f shade(const uniform int shadeMode, const Ray &ray, uniform RTCScene scene)
{
// #if 1
//   if (ray.geomID < 0) return make_vec4f(0.f); 
//   vec3f N = normalize(ray.Ng);
//   // print("N % % %\n",N.x,N.y,N.z);
//   return make_vec4f(abs(dot(N,ray.dir)));
// #endif
  if (shadeMode == 0)
    return shade_eyeLight(ray);
  if (shadeMode == 1)
    return shade_primID(ray);
  if (shadeMode == 2)
    return shade_geomID(ray);
  if (shadeMode == 3)
    return shade_instID(ray);
  if (shadeMode == 4)
    return shade_Ng(ray);
  if (shadeMode == 5)
    return shade_eyeLight_testShadow(ray,scene);
  else
    print("unknown shade mode % in raycast renderer",shadeMode);
}

// //! render a line with ray casting; 
// /*! \internal this should eventually get moved to a tile-based
//   renderer (ideally with load balancer support for distributed
//   rendering), but right now lines is easier */
// task void RayCastRenderer_renderLine(uniform FrameBuffer *uniform fb,
//                                      uniform RTCScene scene,
//                                      uniform Camera *uniform camera,
//                                      const uniform int shadeMode)
// {
//   const uniform int size_x = fb->size.x;
//   const uniform int size_y = fb->size.y;
//   const uniform int y = taskIndex;
//   foreach (x = 0 ... size_x) {
//     const float screen_u = (x+.5f)/size_x;
//     const float screen_v = (y+.5f)/size_y;
//     Ray ray;
//     camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
//     // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
// #if 0
//     rtcIntersect(scene,(varying RTCRay&)ray);
// #else
//     rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
// #endif
    
//     const vec4f col = shade(shadeMode,ray,scene);
//     fb->setPixel(fb,make_vec2i(x,y),col);
//   }
// }



export void ispc__RayCastRenderer_renderTile(void *uniform _tile, //uniform Tile *uniform tile,
                                             void *uniform _camera, //uniform Camera *uniform camera,
                                             uniform RTCScene scene,
                                             const uniform int shadeMode)
{
#if 0
  print("render tile \n");
  return;
#else
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
      // uniform bool dbg = any(x==512 && y==512);

      // print("+pixel % %\n",x,y);
      const float screen_u = (x+.5f)/size_x;
      const float screen_v = (y+.5f)/size_y;
      Ray ray;
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
      // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
      // if (dbg)
      //  print("ray.dir\n %\n %\n %\n",ray.dir.x,ray.dir.y,ray.dir.z);
#if 0
      rtcIntersect(scene,(varying RTCRay&)ray);
#else
      rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
#endif
      // if (dbg)
      //   print("hit\n %\n %\n %\n",ray.primID,ray.geomID,ray.instID);
      vec4f col = shade(shadeMode,ray,scene);
      // if (dbg)
      //   print("col\n %\n %\n %\n",col.x,col.y,col.z);

      // if (ray.geomID >= 0)
      //   print("col % % %\n",col.x,col.y,col.z);



      // col = make_vec4f(make_random_color(i));

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
#endif
}

