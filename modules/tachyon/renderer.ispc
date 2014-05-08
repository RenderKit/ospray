/*! \file tachyon.ispc Defines ISPC side of the tachyon renderer */

// ospray stuff
#include "ospray/fb/framebuffer.ih"
#include "ospray/camera/perspectivecamera.ih"
#include "ospray/common/model.ih"
#include "ospray/render/util.ih"

struct Phong {
  float plastic;
  float size;
  vec3f color;
  int texFunc;
};
struct Texture {
  float ambient, diffuse, specular, opacity;
  Phong phong;
};

inline vec4f shade(const Ray &ray, uniform Model *uniform model)
{ 
  if (ray.geomID < 0) return make_vec4f(0.f); 
  DifferentialGeometry dg;
  postIntersect(model,dg,ray,DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD);

  float c = dot(dg.Ns,ray.dir); 

  Ray shadowRay;
  const vec3f P = ray.org + (ray.t *.999f) * ray.dir;
  const vec3f L = make_vec3f(-1,2,-.5);

  setRay(shadowRay,P,L);
  const bool occluded = isOccluded(model,shadowRay);
  return make_vec4f(occluded 
                    ? .2f+.2f*abs(c)
                    : .3f+.8f*abs(c));
}

export void TachyonRenderer_renderTile(void *uniform _tile,
                                             void *uniform _camera,
                                             void *uniform _model,
                                             void *uniform _textureArray)
{
  uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;
  uniform Model  *uniform model  = (uniform Model *uniform)_model;
  uniform Texture *uniform textureArray = (uniform Texture *uniform)_textureArray;

  tile->format = TILE_FORMAT_RGBA8 | TILE_FORMAT_FLOAT4;
  const uniform uint32 size_x = tile->fbSize.x;
  const uniform uint32 size_y = tile->fbSize.y;
  const uniform uint32 x0 = tile->region.lower.x;
  const uniform uint32 y0 = tile->region.lower.y;
  uniform RTCScene scene = model->embreeSceneHandle;
  for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32 frag = i*programCount+programIndex;
    const uint32  x     = x0 + (frag % TILE_SIZE);
    const uint32  y     = y0 + (frag / TILE_SIZE);
    if (x < size_x & y < size_y) {
      const float screen_u = (x+.5f)/size_x;
      const float screen_v = (y+.5f)/size_y;
      Ray ray;
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
      traceRay(model,ray);
      const vec4f col = shade(ray,model);
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.t;
      tile->rgba8[i] = cvt_uint32(col);
    }
  }
}

