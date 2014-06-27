/*! \file raycast.ispc Defines ISPC side of the ray cast renderer */

// ospray stuff
#include "../../fb/framebuffer.ih"
#include "../../camera/perspectivecamera.ih"
#include "ospray/common/model.ih"
#include "../util.ih"
#include "ospray/render/renderer.ih"

struct RayCastRenderer
{
  uniform Renderer inherited;
  int shadeMode;
};

inline vec3f shade_Ng(const Ray &ray)
{ 
  if (ray.geomID < 0) return make_vec3f(0.f); 
  return make_vec3f(absf(normalize(ray.Ng))); 
}

inline vec3f shade_primID(const Ray &ray)
{ 
  return make_vec3f(make_random_color(ray.primID)); 
}

inline vec3f shade_geomID(const Ray &ray)
{ 
  return make_vec3f(make_random_color(ray.geomID)); 
}

inline vec3f shade_instID(const Ray &ray)
{ 
  return make_vec3f(make_random_color(ray.instID)); 
}

inline vec3f shade_eyeLight(const Ray &ray)
{ 
  if (ray.geomID < 0) return make_vec3f(0.f); 
  const float c = dot(normalize(ray.Ng),ray.dir); 
  return make_vec3f(.3f+.8f*abs(c));
}

inline vec3f shade_eyeLight_testShadow(const Ray &ray, uniform Model *uniform model)
{ 
  if (ray.geomID < 0) return make_vec3f(0.f); 
  float c = dot(normalize(ray.Ng),ray.dir); 

  Ray shadowRay;
  const vec3f P = ray.org + (ray.t *.999f) * ray.dir;
  const vec3f L = make_vec3f(-1,2,-.5);
  setRay(shadowRay,P,L);
  bool occluded = isOccluded(model,shadowRay);
  return make_vec3f(occluded 
                    ? .2f+.2f*abs(c)
                    : .3f+.8f*abs(c));
}

void RayCastRenderer_renderSample_eyeLight(uniform Renderer *uniform _renderer,
                                           varying ScreenSample &sample)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;
  // print("ray.org\n %\n %\n %\n",sample.ray.org.x,sample.ray.org.y,sample.ray.org.z);
  // print("ray.dir\n %\n %\n %\n",sample.ray.dir.x,sample.ray.dir.y,sample.ray.dir.z);
  traceRay(renderer->inherited.model,sample.ray);
  // print("geomid %\n",sample.ray.geomID);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;
  sample.rgb   = shade_eyeLight(sample.ray);
}

export void *uniform RayCastRenderer_create_eyeLight(void *uniform cppE,
                                                     void *uniform _model,
                                                     void *uniform _camera)
{
  uniform RayCastRenderer *uniform renderer = uniform new uniform RayCastRenderer;
  Renderer_constructor(&renderer->inherited,cppE,_model,_camera);
  renderer->inherited.renderSample = RayCastRenderer_renderSample_eyeLight;
  return renderer;
}

export void RayCastRenderer_destroy(void *uniform _renderer)
{
  uniform RayCastRenderer *uniform renderer = (uniform RayCastRenderer *uniform)_renderer;
  delete renderer;
}
