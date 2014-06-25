
/*! \file raycast.ispc Defines ISPC side of the ray cast renderer */

// ospray stuff
#include "ospray/fb/framebuffer.ih"
#include "ospray/camera/perspectivecamera.ih"
#include "ospray/render/util.ih"
#include "ospray/common/model.ih"
#include "ospray/texture/texture2d.ih"
#include "ospray/lights/light.ih"
#include "ospray/lights/directionallight.ih"
// obj renderer
#include "objmaterial.ih"
#include "objpointlight.ih"

#define EPS (1e-4f)
#define ALPHA_THRESHOLD (.05f)

struct OBJRenderer {
  uniform Model *model;
  const uniform OBJPointLight *uniform *uniform pointLights;
  uint32        numPointLights;
  const uniform DirectionalLight *uniform *uniform dirLights;
  uint32        numDirLights;
};

inline float lightAlpha(Ray &ray, uniform Model *uniform model, const float weight) {
  float alpha = 1.f;
  int max_depth = 8;
  const float org_t_max = ray.t;

  while (1) {
    traceRay(model,ray);

    if (ray.geomID < 0) return alpha;

    DifferentialGeometry dg;
    postIntersect(model, dg, ray, DG_MATERIALID | DG_TEXCOORD);

    uniform Material *material = dg.material;
    uniform OBJMaterial *objMaterial = (uniform OBJMaterial *)material;

    float material_opacity = 1;
    foreach_unique( mat in objMaterial ) {
      const vec3f opacity_map = mat->map_d ? mat->map_d->get(mat->map_d, dg.st) : make_vec3f(1.f);
      material_opacity = mat->map_d ? opacity_map.x : mat->d;
    }

    alpha = alpha * (1.f - material_opacity );

    if (alpha * weight < ALPHA_THRESHOLD) return alpha;

    max_depth--;
    if (max_depth <= 0) return alpha;

    ray.t0 = ray.t + EPS;
    ray.t = org_t_max;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

inline vec3f OBJRenderer_shadeRay(Ray &ray, const uniform OBJRenderer *uniform renderer) 
{ 
  // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
  // print("ray.dir % % %\n",ray.dir.x,ray.dir.y,ray.dir.z);
  vec3f color = make_vec3f(0.f);
  float path_opacity = 1.f;
  int max_depth = 10;
  uniform int depth = 0;

  while (1) {
    traceRay(renderer->model,ray);

    if (ray.geomID < 0) {
      const vec3f bg_color = make_vec3f(0.f);

      color = color + path_opacity * bg_color;
      return color;
    }

    vec3f local_shade_color = make_vec3f(0.f);

    DifferentialGeometry dg;
    postIntersect(renderer->model,
        dg,
        ray,
        DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

    uniform Material *material = dg.material;
    uniform OBJMaterial *objMaterial = (uniform OBJMaterial *)material;

    if (!objMaterial) {
      color = color + path_opacity * make_vec3f(dg.color);
      return color;
    }

    float d = 1.f;
    float Ns = 0.f;
    vec3f Kd = make_vec3f(0.f,1.f,0.f);
    vec3f Ks = make_vec3f(0.f);
    vec3f bump = make_vec3f(0.f);

    foreach_unique (mat in objMaterial) {
      const vec3f opacity_map = mat->map_d ? mat->map_d->get(mat->map_d, dg.st) : make_vec3f(1.f);
      d = mat->map_d ? opacity_map.x : mat->d;
      Kd = mat->map_Kd ? mat->map_Kd->get(mat->map_Kd, dg.st) : mat->Kd;
      Ks = mat->map_Ks ? mat->map_Ks->get(mat->map_Ks, dg.st) : mat->Ks;
      const vec3f spec_exp_map = mat->map_Ns ? mat->map_Ns->get(mat->map_Ns, dg.st) : make_vec3f(0.f);
      Ns = mat->map_Ns ? spec_exp_map.x : mat->Ns;
      bump = mat->map_Bump ? mat->map_Bump->get(mat->map_Bump, dg.st) : make_vec3f(0.f);
    }

    const float local_opacity = path_opacity * d;

    max_depth-=1;
    if (max_depth <= 0) return color;

    if (local_opacity > 0.01f) { //worth shading
      const vec3f R = ray.dir - (2.f * dot(ray.dir, dg.Ns))*dg.Ns;
      const vec3f P = ray.org + (ray.t * ray.dir * (1.f - EPS)) + EPS * dg.Ng;

      //Some fake hard coded ambient light
      color = color + local_opacity * Kd * .25;

      //calculate shading for point lights
      for (uniform int i = 0; i < renderer->numPointLights && renderer->pointLights; i++) {
        const uniform OBJPointLight *uniform l = (const uniform OBJPointLight *uniform)renderer->pointLights[i];
        float dist;
        const vec3f L = normalize(l->base.position - P, dist);
        if (dist > l->base.range) continue;
        const float attenuation = l->constantAttenuation
                                + l->linearAttenuation * dist
                                + l->quadraticAttenuation * dist * dist;
        const float cosNL = abs(dot(L,dg.Ns));
        const float cosLR = max(0.f,dot(L,R));
        const vec3f unshaded_light_contrib = local_opacity * (Kd * cosNL + Ks * powf(cosLR, Ns)) * l->base.color * rcpf(attenuation);
        const float max_contrib = reduce_max(unshaded_light_contrib);
        if (max_contrib > .01f) {
          Ray shadowRay;
          setRay(shadowRay,P,L);
          const float light_alpha = lightAlpha(shadowRay, renderer->model, max_contrib);

          local_shade_color = local_shade_color + light_alpha * unshaded_light_contrib;
        }
      }

      //calculate shading for directional lights
      for (uniform int i = 0; i < renderer->numDirLights && renderer->dirLights; i++) {
        const uniform DirectionalLight *uniform l = (const uniform DirectionalLight *uniform)renderer->dirLights[i];
        const vec3f L = normalize(neg(l->direction));

        const float cosNL = abs(dot(L, dg.Ns));
        const float cosLR = max(0.f, dot(L,R));

        const vec3f unshaded_light_contrib = local_opacity * (Kd * cosNL + Ks * powf(cosLR, Ns)) * l->color;

        const float max_contrib = reduce_max(unshaded_light_contrib);
        if (max_contrib > .01f) {
          Ray shadowRay;
          setRay(shadowRay,P,L);
          const float light_alpha = lightAlpha(shadowRay, renderer->model, max_contrib);

          local_shade_color = local_shade_color + light_alpha * unshaded_light_contrib;
        }
      }

      color = color + local_shade_color;
    }

    path_opacity = path_opacity * (1.f - d);
    if (path_opacity < 0.01f) return color; //remaining contribution too low kill path

    ray.t0 = ray.t + EPS;
    ray.t = infinity;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }

  return color;
}

export void ispc__OBJRenderer_renderTile(void *uniform _tile, //uniform Tile *uniform tile,
                                         void *uniform _camera, //uniform Camera *uniform camera,
                                         void *uniform _model,
                                         uniform int num_point_lights,
                                         void *uniform *uniform _point_lights,
                                         uniform int num_dir_lights,
                                         void *uniform *uniform _dir_lights)
{
  uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;

  uniform OBJRenderer renderer;
  renderer.model   = (uniform Model *uniform)_model;
  renderer.pointLights = (const uniform OBJPointLight *uniform *uniform)_point_lights;
  renderer.numPointLights = num_point_lights;
  renderer.dirLights = (const uniform DirectionalLight *uniform *uniform)_dir_lights;
  renderer.numDirLights = num_dir_lights;

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
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v),
                      (varying vec2f)make_vec2f(0.f,0.f));
      const vec3f col = OBJRenderer_shadeRay(ray,&renderer);
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

