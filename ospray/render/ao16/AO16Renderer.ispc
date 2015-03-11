// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray 
#include "ospray/fb/FrameBuffer.ih"
#include "ospray/camera/Camera.ih"
#include "ospray/common/Ray.ih"
#include "ospray/render/util.ih"
#include "ospray/math/math.ih"
#include "ospray/common/Model.ih"
#include "ospray/render/Renderer.ih"
#include "ospray/texture/Texture2D.ih"

#define EPS 1e-5f

uniform float SimpleAO__randomU[16][16] = {
  {0, 0.5, 0.25, 0.75, 0.125, 0.625, 0.375, 0.875, 0.0625, 0.5625, 0.3125, 0.8125, 0.1875, 0.6875, 0.4375, 0.9375},
  {0.03125, 0.53125, 0.28125, 0.78125, 0.15625, 0.65625, 0.40625, 0.90625, 0.09375, 0.59375, 0.34375, 0.84375, 0.21875, 0.71875, 0.46875, 0.96875},
  {0.015625, 0.515625, 0.265625, 0.765625, 0.140625, 0.640625, 0.390625, 0.890625, 0.078125, 0.578125, 0.328125, 0.828125, 0.203125, 0.703125, 0.453125, 0.953125},
  {0.046875, 0.546875, 0.296875, 0.796875, 0.171875, 0.671875, 0.421875, 0.921875, 0.109375, 0.609375, 0.359375, 0.859375, 0.234375, 0.734375, 0.484375, 0.984375},
  {0.0078125, 0.507812, 0.257812, 0.757812, 0.132812, 0.632812, 0.382812, 0.882812, 0.0703125, 0.570312, 0.320312, 0.820312, 0.195312, 0.695312, 0.445312, 0.945312},
  {0.0390625, 0.539062, 0.289062, 0.789062, 0.164062, 0.664062, 0.414062, 0.914062, 0.101562, 0.601562, 0.351562, 0.851562, 0.226562, 0.726562, 0.476562, 0.976562},
  {0.0234375, 0.523438, 0.273438, 0.773438, 0.148438, 0.648438, 0.398438, 0.898438, 0.0859375, 0.585938, 0.335938, 0.835938, 0.210938, 0.710938, 0.460938, 0.960938},
  {0.0546875, 0.554688, 0.304688, 0.804688, 0.179688, 0.679688, 0.429688, 0.929688, 0.117188, 0.617188, 0.367188, 0.867188, 0.242188, 0.742188, 0.492188, 0.992188},
  {0.00390625, 0.503906, 0.253906, 0.753906, 0.128906, 0.628906, 0.378906, 0.878906, 0.0664062, 0.566406, 0.316406, 0.816406, 0.191406, 0.691406, 0.441406, 0.941406},
  {0.0351562, 0.535156, 0.285156, 0.785156, 0.160156, 0.660156, 0.410156, 0.910156, 0.0976562, 0.597656, 0.347656, 0.847656, 0.222656, 0.722656, 0.472656, 0.972656},
  {0.0195312, 0.519531, 0.269531, 0.769531, 0.144531, 0.644531, 0.394531, 0.894531, 0.0820312, 0.582031, 0.332031, 0.832031, 0.207031, 0.707031, 0.457031, 0.957031},
  {0.0507812, 0.550781, 0.300781, 0.800781, 0.175781, 0.675781, 0.425781, 0.925781, 0.113281, 0.613281, 0.363281, 0.863281, 0.238281, 0.738281, 0.488281, 0.988281},
  {0.0117188, 0.511719, 0.261719, 0.761719, 0.136719, 0.636719, 0.386719, 0.886719, 0.0742188, 0.574219, 0.324219, 0.824219, 0.199219, 0.699219, 0.449219, 0.949219},
  {0.0429688, 0.542969, 0.292969, 0.792969, 0.167969, 0.667969, 0.417969, 0.917969, 0.105469, 0.605469, 0.355469, 0.855469, 0.230469, 0.730469, 0.480469, 0.980469},
  {0.0273438, 0.527344, 0.277344, 0.777344, 0.152344, 0.652344, 0.402344, 0.902344, 0.0898438, 0.589844, 0.339844, 0.839844, 0.214844, 0.714844, 0.464844, 0.964844},
  {0.0585938, 0.558594, 0.308594, 0.808594, 0.183594, 0.683594, 0.433594, 0.933594, 0.121094, 0.621094, 0.371094, 0.871094, 0.246094, 0.746094, 0.496094, 0.996094}
};


uniform float SimpleAO__randomV[16][16] = {
{0, 0.333333, 0.666667, 0.111111, 0.444444, 0.777778, 0.222222, 0.555556, 0.888889, 0.037037, 0.37037, 0.703704, 0.148148, 0.481482, 0.814815, 0.259259},
{0.592593, 0.925926, 0.0740741, 0.407407, 0.740741, 0.185185, 0.518519, 0.851852, 0.296296, 0.62963, 0.962963, 0.0123457, 0.345679, 0.679012, 0.123457, 0.45679},
{0.790123, 0.234568, 0.567901, 0.901235, 0.0493827, 0.382716, 0.716049, 0.160494, 0.493827, 0.82716, 0.271605, 0.604938, 0.938272, 0.0864198, 0.419753, 0.753086},
{0.197531, 0.530864, 0.864198, 0.308642, 0.641975, 0.975309, 0.0246914, 0.358025, 0.691358, 0.135802, 0.469136, 0.802469, 0.246914, 0.580247, 0.91358, 0.0617284},
{0.395062, 0.728395, 0.17284, 0.506173, 0.839506, 0.283951, 0.617284, 0.950617, 0.0987654, 0.432099, 0.765432, 0.209877, 0.54321, 0.876543, 0.320988, 0.654321},
{0.987654, 0.00411523, 0.337449, 0.670782, 0.115226, 0.44856, 0.781893, 0.226337, 0.559671, 0.893004, 0.0411523, 0.374486, 0.707819, 0.152263, 0.485597, 0.81893},
{0.263375, 0.596708, 0.930041, 0.0781893, 0.411523, 0.744856, 0.1893, 0.522634, 0.855967, 0.300412, 0.633745, 0.967078, 0.0164609, 0.349794, 0.683128, 0.127572},
{0.460905, 0.794239, 0.238683, 0.572016, 0.90535, 0.053498, 0.386831, 0.720165, 0.164609, 0.497942, 0.831276, 0.27572, 0.609053, 0.942387, 0.090535, 0.423868},
{0.757202, 0.201646, 0.534979, 0.868313, 0.312757, 0.646091, 0.979424, 0.0288066, 0.36214, 0.695473, 0.139918, 0.473251, 0.806584, 0.251029, 0.584362, 0.917696},
{0.0658436, 0.399177, 0.73251, 0.176955, 0.510288, 0.843621, 0.288066, 0.621399, 0.954733, 0.102881, 0.436214, 0.769547, 0.213992, 0.547325, 0.880658, 0.325103},
{0.658436, 0.99177, 0.00823045, 0.341564, 0.674897, 0.119342, 0.452675, 0.786008, 0.230453, 0.563786, 0.897119, 0.0452675, 0.378601, 0.711934, 0.156379, 0.489712},
{0.823045, 0.26749, 0.600823, 0.934156, 0.0823045, 0.415638, 0.748971, 0.193416, 0.526749, 0.860082, 0.304527, 0.63786, 0.971193, 0.0205761, 0.353909, 0.687243},
{0.131687, 0.465021, 0.798354, 0.242798, 0.576132, 0.909465, 0.0576132, 0.390947, 0.72428, 0.168724, 0.502058, 0.835391, 0.279835, 0.613169, 0.946502, 0.0946502},
{0.427984, 0.761317, 0.205761, 0.539095, 0.872428, 0.316872, 0.650206, 0.983539, 0.0329218, 0.366255, 0.699588, 0.144033, 0.477366, 0.8107, 0.255144, 0.588477},
{0.921811, 0.0699589, 0.403292, 0.736625, 0.18107, 0.514403, 0.847737, 0.292181, 0.625514, 0.958848, 0.106996, 0.440329, 0.773663, 0.218107, 0.55144, 0.884774},
{0.329218, 0.662551, 0.995885, 0.00137174, 0.334705, 0.668038, 0.112483, 0.445816, 0.77915, 0.223594, 0.556927, 0.890261, 0.0384088, 0.371742, 0.705075, 0.14952}
};

uniform vec3f AO16_directions[16][16];
uniform bool AO16_directions_initialized = false;

struct AO16Renderer {
  uniform Renderer inherited;
  uniform vec3f bgColor;
  uniform int samplesPerFrame;
};

inline vec3f getShadingNormal(const Ray &ray)
{
  vec3f N = ray.Ng;
  float f = rcp(sqrt(dot(N,N)));
  if (dot(N,ray.dir) >= 0.f) f = -f;
  return f*N;
}

void getBinormals(vec3f &biNorm0, vec3f &biNorm1, const vec3f &gNormal)
{
  biNorm0 = make_vec3f(1.f,0.f,0.f);
  if (abs(dot(biNorm0,gNormal)) > .95f)
    biNorm0 = make_vec3f(0.f,1.f,0.f);
  biNorm1 = normalize(cross(biNorm0,gNormal));
  biNorm0 = normalize(cross(biNorm1,gNormal));
}

struct AO16Material {
  Material inherited;
  vec3f Kd;
  uniform Texture2D *uniform map_Kd;
};

export void *uniform AO16Material_create(void *uniform cppE)
{
  uniform AO16Material *uniform mat = uniform new uniform AO16Material;
  mat->inherited.cppEquivalent = cppE;
  return mat;
}

export void AO16Material_set(void *uniform _mat, 
                             const uniform vec3f &Kd,
                             void *uniform _map_Kd
                             )
{
  uniform AO16Material *uniform mat = (uniform AO16Material *uniform)_mat;
  mat->Kd = Kd;
  mat->map_Kd = (uniform Texture2D *uniform)_map_Kd;
}

inline float rotate(float x, float dx)
{
  x += dx;
  if (x >= 1.f) x -= 1.f;
  return x;
}

inline uniform float rotate(uniform float x, uniform float dx)
{
  x += dx;
  if (x >= 1.f) x -= 1.f;
  return x;
}

inline vec3f getRandomDir(const int j, const int i, 
                          const vec3f biNorm0,
                          const vec3f biNorm1, 
                          const vec3f gNormal,
                          const float rot_x, const float rot_y)
{
  const float r0 = rotate(SimpleAO__randomU[j][i],rot_x);
  const float r1 = rotate(SimpleAO__randomV[j][i],rot_y);
        
  const float w = sqrt(1.f-r1);
  const float x = cos((2.f*M_PI)*r0)*w;
  const float y = sin((2.f*M_PI)*r0)*w;
  const float z = sqrt(r1)+EPS;
  return x*biNorm0+y*biNorm1+z*gNormal;
}


inline vec3f shade_ao16(uniform AO16Renderer *uniform renderer,
                        int sampleOfs, int sampleCnt,
                        const Ray &ray, 
                        const int32 pixel_x, const int32 pixel_y,
                        const float rot_x, const float rot_y)
{
  if (ray.geomID < 0)
    return renderer->bgColor;

  DifferentialGeometry dg;
  postIntersect(renderer->inherited.model,dg,ray,
                DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD
                |DG_MATERIALID|DG_COLOR|DG_TEXCOORD);
  
  uniform AO16Material *mat = ((uniform AO16Material*)dg.material);
  vec3f baseColor = make_vec3f(1.f);
  if (mat) {
    foreach_unique(m in mat) {
      baseColor = m->Kd;
      if (m->map_Kd) {
        vec4f Kd_from_map = get4f(m->map_Kd,dg.st);
        baseColor = baseColor * make_vec3f(Kd_from_map);
      }
    }
  }
  // should be done in material:
  baseColor = baseColor * make_vec3f(dg.color);
  
  int hits = 0;
  vec3f biNormU,biNormV;
  const vec3f N = dg.Ns;
  getBinormals(biNormU,biNormV,N);

  const int sindex_x = pixel_x & 0x3;
  const int sindex_y = pixel_y & 0x3;
  const int sindex = (sindex_x) * 4 + (sindex_y);
  for (uniform int i=0;i<sampleCnt;i++) {
    const vec3f ao_dir
      = getRandomDir(sindex,
                     (3*sindex_x+5*sindex_y+i+sampleCnt*sampleOfs)%16,
                     biNormU,biNormV,N,rot_x,rot_y);
    
    Ray ao_ray;
    setRay(ao_ray,dg.P+1e-3f*N,ao_dir);
    ao_ray.t0 = EPS;
    ao_ray.t = 1e20f; 
    if (dot(ao_ray.dir,N) < 0.05f || isOccluded(renderer->inherited.model,ao_ray))
      hits++;
  }
  float diffuse = absf(dot(N,ray.dir));
  float frac_visible = (sampleCnt-hits) * (1.0f/sampleCnt);
  vec3f rgb = baseColor * make_vec3f(diffuse * min(.01f + 1.1f * frac_visible,1.f));
  return rgb;
}


void AO16Renderer_renderSample(uniform Renderer *uniform _renderer,
                               varying ScreenSample &sample)
{
  uniform AO16Renderer *uniform renderer = (uniform AO16Renderer *uniform)_renderer;

  traceRay(renderer->inherited.model,sample.ray);
  sample.z     = sample.ray.t;
  sample.alpha = 1.f;

  uniform int samplesPerFrame = renderer->samplesPerFrame;
  uniform int framesPer16 = 16/samplesPerFrame;
  int accumID = sample.sampleID.z / framesPer16;
  int sampleOfs = sample.sampleID.z % framesPer16;
  float rot_x = 1.f-precomputedHalton3(accumID);
  float rot_y = 1.f-precomputedHalton5(accumID);
  sample.rgb   = shade_ao16(renderer,
                            sampleOfs,samplesPerFrame,
                            sample.ray,
                            sample.sampleID.x,
                            sample.sampleID.y, 
                            rot_x,rot_y);
}


export void *uniform AO16Renderer_create(void *uniform cppE,
                                         void *uniform _model,
                                         void *uniform _camera)
{
  uniform AO16Renderer *uniform renderer = uniform new uniform AO16Renderer;
  Renderer_constructor(&renderer->inherited,cppE,_model,_camera);
  renderer->inherited.renderSample = AO16Renderer_renderSample;
  return renderer;
}

export void AO16Renderer_set(void *uniform _renderer,
                             uniform int samplesPerFrame,
                             const uniform vec3f &bgColor,
                             void *uniform _model,
                             void *uniform _camera)
{
  uniform AO16Renderer *uniform renderer = (uniform AO16Renderer *uniform)_renderer;
  renderer->bgColor = bgColor;
  renderer->samplesPerFrame = samplesPerFrame;
  renderer->inherited.model = (uniform Model*)_model;
  renderer->inherited.camera = (uniform Camera*)_camera;
}

export void AO16Renderer_destroy(void *uniform _renderer)
{
  uniform AO16Renderer *uniform renderer = (uniform AO16Renderer *uniform)_renderer;
  delete renderer;
}

