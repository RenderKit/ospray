/*! \file xray_renderer.ispc \brief ISPC-version of renderTile
  function for ospray::mhtk::XRayRenderer */

// ospray stuff
#include "ospray/fb/framebuffer.ih"
#include "ospray/camera/perspectivecamera.ih"
#include "ospray/common/ray.ih"
#include "ospray/render/util.ih"
#include "multihit_kernel.ih"

#define MAX_HITS_PER_TRACE 100

inline vec4f XRayRenderer_trace(Ray &ray, RTCScene scene)
{
  MHTKHit hitArray[MAX_HITS_PER_TRACE];
  const int32 numHits = multiHitKernel(scene,ray,hitArray,MAX_HITS_PER_TRACE);
#if 0
  return make_vec4f(make_random_color(numHits));
#else
  float color = 0.f;
  float addtl = .5f;
  for (int i=0;i<numHits;i++) {
    vec3f N = normalize(hitArray[i].Ng);
    color += (addtl*abs(dot(ray.dir,N)));
    addtl *= .5f;
  }
  return make_vec4f(color);
#endif
}

export void ispc__mhtk_XRayRenderer_renderTile(void *uniform _tile,
                                               void *uniform _camera,
                                               RTCScene scene)
{
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;
  uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;

  tile->format = TILE_FORMAT_RGBA8 | TILE_FORMAT_FLOAT4;
  const uniform uint32 size_x = tile->fbSize.x;
  const uniform uint32 size_y = tile->fbSize.y;
  const uniform uint32 x0 = tile->region.lower.x;
  const uniform uint32 y0 = tile->region.lower.y;
  for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32 frag = i*programCount+programIndex;
    const uint32  x     = x0 + (frag % TILE_SIZE);
    const uint32  y     = y0 + (frag / TILE_SIZE);
    if (x < size_x & y < size_y) {
      const float screen_u = (x+.5f)*tile->rcp_fbSize.x;
      const float screen_v = (y+.5f)*tile->rcp_fbSize.y;
      Ray ray;
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));

      const vec4f col = XRayRenderer_trace(ray,scene);
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.t;
      tile->rgba8[i] = cvt_uint32(col);
    }
  }

}
