
/*! \file raycast.ispc Defines ISPC side of the ray cast renderer */

// ospray stuff
#include "../../fb/framebuffer.ih"
#include "../../camera/perspectivecamera.ih"
#include "../util.ih"
#include "ospray/common/model.ih"
#include "objmaterial.ih"

inline vec3f OBJRenderer_shadeRay(Ray &ray, uniform Model *uniform model)
{ 
  // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
  // print("ray.dir % % %\n",ray.dir.x,ray.dir.y,ray.dir.z);
  rtcIntersect(model->embreeSceneHandle,*((varying RTCRay *uniform)&ray));
  if (ray.geomID < 0)
    return make_vec3f(0.f);

  uniform Material *material = model->geometry[ray.geomID]->material;
  uniform OBJMaterial *objMaterial = (uniform OBJMaterial *)material;

  vec3f Kd = make_vec3f(0,1,0);
  if (objMaterial) {
    Kd = objMaterial->Kd;
  }

  float c = dot(normalize(ray.Ng),ray.dir); 

  Ray shadowRay;
  const vec3f P = ray.org + (ray.t *.999f) * ray.dir;
  const vec3f L = make_vec3f(+1,1.3,-.5);
  setRay(shadowRay,P,L);
  rtcOccluded(model->embreeSceneHandle,*((varying RTCRay *uniform)&shadowRay));
  const bool occluded = shadowRay.geomID >= 0;
  return Kd * make_vec3f(occluded 
                         ? .2f+.2f*abs(c)
                         : .3f+1.2f*abs(c));
}

export void ispc__OBJRenderer_renderTile(void *uniform _tile, //uniform Tile *uniform tile,
                                         void *uniform _camera, //uniform Camera *uniform camera,
                                         uniform Model *uniform model)
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
      const vec3f col = OBJRenderer_shadeRay(ray,model);
#if 1
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = 1.f;
      tile->z[i] = ray.t;
      tile->rgba8[i] = cvt_uint32(col);
#endif
    }
  }
}

