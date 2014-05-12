/*! \file tachyon.ispc Defines ISPC side of the tachyon renderer */

// ospray stuff
#include "ospray/fb/framebuffer.ih"
#include "ospray/camera/perspectivecamera.ih"
#include "ospray/common/model.ih"
#include "ospray/render/util.ih"

struct Phong {
  float plastic;
  float size;
};
struct Texture {
  float ambient, diffuse, specular, opacity;
  Phong phong;
  vec3f color;
  int texFunc;
};
struct PointLight 
{
  vec3f center;
  vec3f color;
  struct {
    float constant, linear, quadratic;
  } att;
};
struct DirLight 
{
  vec3f color;
  vec3f dir;
};

struct Tachyon {
  uniform Model *model;
  uniform Texture *textureArray;
  uniform PointLight *pointLight;
  uint32  numPointLights;
  uniform DirLight *dirLight;
  uint32  numDirLights;
};

#define EPS (1e-4f)
#define ALPHA_THRESHOLD (.05f)

inline float reduce_max(const vec3f v) { return max(max(v.x,v.y),v.z); }

inline float lightAlpha(Ray &ray, uniform Tachyon *uniform tachyon, const float weight)
{
  // return 1.f; 

  float alpha = 1.f;
  int max_depth = 8; // max number of rays to be traced...
  float org_t_max = ray.t;
  while (1) {
    traceRay(tachyon->model,ray);
    if (ray.geomID < 0) 
      return alpha;
    
    DifferentialGeometry dg;
    postIntersect(tachyon->model,dg,ray,
                  DG_MATERIALID
                  );
    const float material_opacity = tachyon->textureArray[dg.materialID].opacity;
    alpha = alpha * (1.f-material_opacity);

    // if (material_opacity > .6f) {
    //   int one = 1;
    //   print("opacity %\n %\n %\n",one,material_opacity,alpha);
    // }

    if (alpha * weight < ALPHA_THRESHOLD) return alpha;

    max_depth--;
    if (max_depth <= 0)
      return alpha;
    
    ray.t0 = ray.t+EPS;
    ray.t  = org_t_max;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

inline vec3f shade(Ray &ray, uniform Tachyon *uniform tachyon)
{ 
  vec3f color = make_vec3f(0.f);
  float path_opacity = 1.f;
  int max_depth = 10; // max number of rays to be traced...
  while (1) {
    traceRay(tachyon->model,ray);
    if (ray.geomID < 0) {
      vec3f bgColor = make_vec3f(0.f);
      color = color + path_opacity * bgColor;
      return color;
    }

    vec3f localShadeColor = make_vec3f(0.f);
    DifferentialGeometry dg;
    postIntersect(tachyon->model,dg,ray,
                  DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD
                  |DG_MATERIALID|DG_COLOR
                  );
    const float material_opacity = tachyon->textureArray[dg.materialID].opacity;
    const float local_opacity = path_opacity * material_opacity;
    
    if (local_opacity > .01f) {
      // enough weight here to shade at all ...
        // print("INVALID MATERIAL ID!\n");

      Texture texture = tachyon->textureArray[dg.materialID];
      if (dg.color.x >= 0.f)
        texture.color = make_vec3f(dg.color);

      // reflective direction, for phong highlight
      const vec3f R = ray.dir - (2.f*dot(ray.dir,dg.Ns))*dg.Ns;

      // -------------------------------------------------------
      // ambient
      // -------------------------------------------------------
      const vec3f P = ray.org+(ray.t*(ray.dir*(1.f-EPS))+EPS*dg.Ng);
      color = color + local_opacity * texture.ambient * texture.color;

#if 1
      // -------------------------------------------------------
      // point lights
      // -------------------------------------------------------
      for (uniform int i=0;i<tachyon->numPointLights;i++) {
        uniform PointLight *uniform l = &tachyon->pointLight[i];
        float dist;
        const vec3f L = normalize(l->center - P,dist);
        const float attenuation
          = l->att.constant+dist*(l->att.linear+dist*l->att.quadratic);
        
        const float cosNL = abs(dot(L,dg.Ns));
        const float cosLR = max(0.f,dot(L,R));
        const float diffuse  = texture.diffuse * cosNL;
        const float specular = (texture.specular+texture.phong.plastic)* powf(cosLR,texture.phong.size);
        const vec3f unshaded_light_contrib
          = local_opacity
          * (diffuse*texture.color+make_vec3f(specular))
          * l->color * rcpf(attenuation);
        
        const float max_contrib = reduce_max(unshaded_light_contrib);
        if (max_contrib < .01f)
          continue;
        
        Ray shadowRay;
        setRay(shadowRay,P,L,EPS,dist*(1.f-EPS)-EPS);
        const float light_alpha = .2f+.8f*lightAlpha(shadowRay,tachyon,max_contrib);

        localShadeColor = localShadeColor + light_alpha * unshaded_light_contrib;
      }
#endif
      // -------------------------------------------------------
      // dir lights
      // -------------------------------------------------------
      for (uniform int i=0;i<tachyon->numDirLights;i++) {
        uniform DirLight *uniform l = &tachyon->dirLight[i];
        const vec3f L = normalize(neg(l->dir));
        
        const float cosNL = abs(dot(L,dg.Ns));
        const float cosLR = max(0.f,dot(L,R));
        // print("cosLR %\n",cosLR);
        const float diffuse  = texture.diffuse * cosNL;
        const float specular = (texture.specular+texture.phong.plastic)* powf(cosLR,texture.phong.size);
        const vec3f unshaded_light_contrib
          = local_opacity
          * (diffuse*texture.color+make_vec3f(specular))
          * l->color;
        
        const float max_contrib = reduce_max(unshaded_light_contrib);
        if (max_contrib < .01f)
          continue;
        
        Ray shadowRay;
        setRay(shadowRay,P,L,EPS,infinity);
        const float light_alpha = .2f+.8f*lightAlpha(shadowRay,tachyon,max_contrib);

        localShadeColor = localShadeColor + light_alpha * unshaded_light_contrib;
      }

      color = color + localShadeColor;
    }

    // done shading this hitpoint, determine opacity of remaing path ...
    path_opacity = path_opacity * (1.f-material_opacity);
    if (path_opacity < .01f) 
      // kill path if remaining contribution too low.
      return color;

    max_depth--;
    if (max_depth <= 0) 
      return color;

    ray.t0 = ray.t+EPS;
    ray.t  = infinity;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }

  return color;
}

export void TachyonRenderer_renderTile(void *uniform _tile,
                                       void *uniform _camera,
                                       void *uniform _model,
                                       void *uniform _textureArray,
                                       void *uniform _pointLight,
                                       uniform uint32 numPointLights,
                                       void *uniform _dirLight,
                                       uniform uint32 numDirLights)
{
  uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;

  uniform Tachyon tachyon;
  tachyon.textureArray = (uniform Texture *uniform)_textureArray;
  tachyon.model   = (uniform Model *uniform)_model;
  tachyon.numPointLights = numPointLights;
  tachyon.pointLight = (uniform PointLight *uniform)_pointLight;
  tachyon.numDirLights = numDirLights;
  tachyon.dirLight = (uniform DirLight *uniform)_dirLight;

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
      const vec4f col = make_vec4f(shade(ray,&tachyon));
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.t;
      tile->rgba8[i] = cvt_uint32(col);
    }
  }
}

