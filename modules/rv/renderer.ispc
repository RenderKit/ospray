/*! \file renderer.ispc Defines ISPC side of RV module renderer */

// ospray stuff
#include "../../ospray/fb/framebuffer.ih"
#include "../../ospray/camera/perspectivecamera.ih"
#include "../../ospray/common/ray.ih"
#include "../../ospray/render/util.ih"

// embree stuff
#include "embree2/rtcore_scene.isph"

export void ispc__RVRenderer_renderTile(void *uniform _tile, 
                                        void *uniform _camera, 
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
    if (x < size_x & y < size_y) {
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

      const vec4f col = make_vec4f(make_random_color(ray.primID)); 
      // if (ray.geomID >= 0)
      //   print("col % % %\n",col.x,col.y,col.z);
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.t;
      tile->rgba8[i] = cvt_uint32(col);
    }
  }  
}

