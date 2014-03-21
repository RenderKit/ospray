/*! \file testrenderer.ispc Defines ISPC side of test frame renderer
    (that only renders a simple test frame */

// ospray stuff
#include "fb/framebuffer.ih"
#include "camera/perspectivecamera.ih"
#include "common/ray.ih"
#include "common/math/bbox.ih"
#include "volume/volume.ih"
#include "render/util.ih"

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
inline vec4f traceRayDVR(const Ray &ray, 
                         uniform Volume *uniform volume,
                         uniform float dt)
{
  const uniform box3f box = make_box3f(make_vec3f(0.f),make_vec3f(1.f)); //volume->clampSize);
  const vec3f rdir = rcp(ray.dir);
  
  vec4f color = make_vec4f(0.f);

  float tenter, texit;
  boxtest(ray, rdir, box, tenter, texit);
  // print("tenter %\n",tenter);
  // print("texit %\n",texit);
  if (tenter >= texit) return color;

  float step_size = dt;//.5;

  float t0, t1;
  float f0 = 0.0;

  t1 = max(floor(tenter / step_size) * step_size, tenter);

  while(t1 < texit && color.w < .97f) {
    // print("t %\n",t1);
    t1 += step_size;

    const vec3f p1 = volume->clampSize * (ray.org + t1 * ray.dir);
    const float f1 = volume->lerpf(volume, p1);

    vec4f color_sample = make_vec4f(f1); //f1 * make_vec4f(1); //, f1);
      
    const float alpha = 1.0 - exp(-f1 * step_size);
    const float alpha_1msa = alpha * (1.0-color.w);
    
    color_sample.w = 1.f;
    color = color + color_sample * alpha_1msa;
  }
  return color;
}

export void ispc__ISPCDVRRenderer_renderTile(void *uniform _tile,
                                             void *uniform _camera,
                                             void *uniform _volume,
                                             uniform float dt)
{
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;
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
    // print("pixel % %\n",x,y);
    if (x < size_x & y < size_y) {
      // print("+pixel % %\n",x,y);
      
      const float screen_u = (x+.5f)*tile->rcp_fbSize.x;
      const float screen_v = (y+.5f)*tile->rcp_fbSize.y;
      Ray ray;
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));

      const vec4f col = traceRayDVR(ray,volume,dt);
      // print("org \n% \n% \n%\n",ray.org.x,ray.org.y,ray.org.z);
      // print("dir \n% \n% \n%\n",ray.dir.x,ray.dir.y,ray.dir.z);
#if 1
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.t;
      tile->rgba8[i] = cvt_uint32(col);
      // print("rgba %\n",tile->rgba8[i]);
#endif
    }
  }

}
