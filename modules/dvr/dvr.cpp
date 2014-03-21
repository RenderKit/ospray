#include "dvr.h"
#include "../camera/perspectivecamera.h"
#include "../volume/volume.h"
// ospray stuff
#include "../../ospray/common/ray.h"

namespace ospray {
  extern "C" void ispc__ISPCDVRRenderer_renderTile(void *tile, void *camera, void *volume);

  TileRenderer::RenderJob *ISPCDVRRenderer::createRenderJob(FrameBuffer *fb)
  {
    Assert(fb && "null frame buffer handle");
    void *_fb = fb->getIE();
    Assert(_fb && "invalid host-side-only frame buffer");

    /* for now, let's get the volume data from the renderer
       parameter. at some point we probably want to have voluems
       embedded in models - possibly with transformation attached to
       them - but for now let's do this as simple as possible */
    Volume *volume = (Volume *)getParam("volume",NULL);
    Assert(volume && "null volume handle in 'dvr' renderer "
           "(did you forget to assign a 'volume' parameter to the renderer?)");
    void *_volume = (void*)volume->getIE();
    Assert(_volume && "invalid volume without a ISPC-side volume "
           "(did you forget to finalize/'commit' the volume?)");

    Camera *camera = (Camera *)getParam("camera",NULL);
    Assert(camera && "null camera handle in 'dvr' renderer "
           "(did you forget to assign a 'camera' parameter to the renderer?)");
    void *_camera = (void*)camera->getIE();
    Assert(_camera && "invalid camera without a ISPC-side camera "
           "(did you forget to finalize/'commit' the camera?)");

    TileJob *tj = new TileJob;
    tj->ispc_camera = _camera;
    tj->ispc_volume = _volume;
    tj->dt          = getParam1f("dt",.0025f);
    return tj;
  }

  TileRenderer::RenderJob *ScalarDVRRenderer::createRenderJob(FrameBuffer *fb)
  {
    Assert(fb && "null frame buffer handle");
    void *_fb = fb->getIE();
    Assert(_fb && "invalid host-side-only frame buffer");

    /* for now, let's get the volume data from the renderer
       parameter. at some point we probably want to have voluems
       embedded in models - possibly with transformation attached to
       them - but for now let's do this as simple as possible */
    Volume *volume = (Volume *)getParam("volume",NULL);
    Assert(volume && "null volume handle in 'dvr' renderer "
           "(did you forget to assign a 'volume' parameter to the renderer?)");

    Camera *camera = (Camera *)getParam("camera",NULL);
    Assert(camera && "null camera handle in 'dvr' renderer "
           "(did you forget to assign a 'camera' parameter to the renderer?)");

    TileJob *tj = new TileJob;
    tj->camera = camera;
    tj->volume = volume;
    tj->dt     = getParam1f("dt",.025f);
    return tj;
  }

  void ISPCDVRRenderer::TileJob::renderTile(Tile &tile)
  {
    ispc__ISPCDVRRenderer_renderTile(&tile,ispc_camera,ispc_volume);
  }


//! performs a ray-box intersection test, and returns intersection interval [t0,t1]
inline void boxtest(const Ray &ray,
                    const vec3f &rdir,
                    const box3f &box,
                    float& t0,
                    float& t1)
{
  const vec3f mins = rdir * (box.lower - (const vec3f&)ray.org); //mul(sub(box.lower, ray.org), rdir);
  const vec3f maxs = rdir * (box.upper - (const vec3f&)ray.org); //mul(sub(box.upper, ray.org), rdir);
  
  t0 = std::max(std::max(ray.t0, 
               std::min(mins.x,maxs.x)),
           std::max(std::min(mins.y,maxs.y),
               std::min(mins.z,maxs.z)));
  
  t1 = std::min(std::min(ray.t, 
               std::max(mins.x,maxs.x)),
           std::min(std::max(mins.y,maxs.y),
               std::max(mins.z,maxs.z)));
}



  //! do direct volume rendering via ray casting 
  /*! \internal this is code adapted from some earlier code that was mostly written by aaron knoll */
  inline vec4f traceRayDVR(const Ray &ray, 
                           Volume *volume,
                           const float dt)
  {
    // PING;
    const box3f bbox = box3f(vec3f(0.f),vec3f(.99999f));
    const vec3f rdir = rcp((const vec3f&)ray.dir);
    // PRINT(bbox);
    // PRINT(ray.org);
    // PRINT(rdir);

    vec4f color = vec4f(0.f);

    float tenter, texit;
    boxtest(ray, rdir, bbox, tenter, texit);

    // PRINT(tenter);
    // PRINT(texit);
    if (tenter >= texit) return color;

    float step_size = dt;

    float t0, t1;
    float f0 = 0.0;

    t1 = std::max(floorf(tenter / step_size) * step_size, tenter);

    // PRINT(t1);
    while(t1 < texit && color.w < .97f) {
      t1 += step_size;

      const vec3fa p1 = ray.org + t1 * ray.dir;
      // PRINT(p1);
      const float f1 = volume->lerpf(p1);

      vec4f color_sample = vec4f(f1); //f1 * make_vec4f(1); //, f1);
      
      const float alpha = 1.0 - exp(-f1 * step_size);
      const float alpha_1msa = alpha * (1.0-color.w);
    
      color_sample.w = 1.f;
      color = color + color_sample * alpha_1msa;
    }
    // PRINT(color);
    return color;
  }

  void ScalarDVRRenderer::TileJob::renderTile(Tile &tile)
  {
    tile.format = TILE_FORMAT_RGBA8 | TILE_FORMAT_FLOAT4;
    const uint32 size_x = tile.fbSize.x;
    const uint32 size_y = tile.fbSize.y;
    const uint32 x0 = tile.region.lower.x;
    const uint32 y0 = tile.region.lower.y;
    for (uint32 frag=0;frag<TILE_SIZE*TILE_SIZE;frag++) {
      const uint32  x     = x0 + (frag % TILE_SIZE);
      const uint32  y     = y0 + (frag / TILE_SIZE);
      if (x < size_x & y < size_y) {
        // print("+pixel % %\n",x,y);
        const float screen_u = (x+.5f)*tile.rcp_fbSize.x;
        const float screen_v = (y+.5f)*tile.rcp_fbSize.y;
        Ray ray;
        camera->initRay(ray,vec2f(screen_u,screen_v));
        const vec4f col = traceRayDVR(ray,volume,dt);
        tile.r[frag] = col.x;
        tile.g[frag] = col.y;
        tile.b[frag] = col.z;
        tile.a[frag] = col.w;
        
        unsigned char *rgba = (unsigned char *)&tile.rgba8[frag];
        rgba[0] = (int)(col.x*256.f);
        rgba[1] = (int)(col.y*256.f);
        rgba[2] = (int)(col.z*256.f);
        rgba[3] = (int)(col.w*256.f);
      }
    }
  }
  OSP_REGISTER_RENDERER(ISPCDVRRenderer,dvr_ispc);
  OSP_REGISTER_RENDERER(ScalarDVRRenderer,dvr_scalar);

  extern "C" void ospray_init_module_dvr() 
  {
    printf("Loaded plugin 'dvr' ...\n");
  }
};

