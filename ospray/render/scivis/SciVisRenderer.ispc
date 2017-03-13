// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "fb/FrameBuffer.ih"
#include "render/util.ih"
#include "common/Model.ih"
#include "texture/Texture2D.ih"
#include "lights/Light.ih"
#include "render/Renderer.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "math/LinearSpace.ih"

#include "SciVisMaterial.ih"

#define ALPHA_THRESHOLD (.05f)

// Data types /////////////////////////////////////////////////////////////////

struct SciVisRenderer
{
  Renderer super;
  float volumeEpsilon;

  const uniform Light *uniform *uniform lights;
  uint32 numLights;
  bool  oneSidedLighting;

  bool  shadowsEnabled;

  int   maxDepth;

  int   aoSamples;
  float aoDistance;
  vec3f aoColor;
  bool  aoTransparencyEnabled;
};

struct SciVisShadingInfo
{
  float d;
  vec3f Kd;
  vec3f Ks;
  float Ns;
  vec3f shadingNormal;

  float local_opacity;
};

inline float lightAlpha(const uniform SciVisRenderer *uniform self,
                        Ray &ray, uniform Model *uniform model,
                        const float weight,
                        const varying float &rayOffset,
                        const varying vec3i &sampleID,
                        const uniform float quality);

inline Volume *
SciVisRenderer_intersectVolumes(const uniform SciVisRenderer *uniform renderer,
                                varying Ray &ray,
                                const varying float &rayOffset);

// Function definitions ///////////////////////////////////////////////////////

inline void initShadingInfo(varying SciVisShadingInfo &info)
{
  info.d  = 1.f;
  info.Ns = 0.f;
  info.Kd = make_vec3f(0.f);
  info.Ks = make_vec3f(0.f);

  info.local_opacity = 0.f;
}

// Material functions //

inline void shadeMaterials(const varying DifferentialGeometry &dg,
                           varying SciVisShadingInfo &info)
{
  uniform SciVisMaterial *scivisMaterial =
      (uniform SciVisMaterial *)dg.material;

  info.shadingNormal = dg.Ns;

  // Get each term from the material //
  if (!scivisMaterial) {
    info.d = dg.color.w;
    info.Kd = make_vec3f(dg.color);
  } else {
    foreach_unique (mat in scivisMaterial) {
      // textures modify (mul) values, see
      //   http://paulbourke.net/dataformats/mtl/
      info.d = mat->d * get1f(mat->map_d, dg.st, 1.f) * dg.color.w;
      info.Kd = mat->Kd * make_vec3f(dg.color);
      if (mat->map_Kd) {
        vec4f Kd_from_map = get4f(mat->map_Kd, dg.st);
        info.Kd = info.Kd * make_vec3f(Kd_from_map);
        info.d *= Kd_from_map.w;
      }
      info.Ks = mat->Ks * get3f(mat->map_Ks, dg.st, make_vec3f(1.f));
      info.Ns = mat->Ns * get1f(mat->map_Ns, dg.st, 1.f);
      // normal mapping
      if (mat->map_Bump) {
        // get normal from texture
        vec3f localNormal = getNormal(mat->map_Bump, dg.st);
        // transform to world space and align to tangents/texture coordinates
        linear3f f = make_LinearSpace3f(normalize(dg.dPds), normalize(dg.dPdt), dg.Ns);
        // in general f is not ortho-normal, thus need to re-normalize
        info.shadingNormal = normalize(f * localNormal);
      }
    }
  }
  // BRDF normalization
  info.Kd = info.Kd * one_over_pi;
  info.Ks = info.Ks * ((info.Ns + 2.f) * one_over_two_pi);
}

// AO functions //

inline float calculateAO(const uniform SciVisRenderer *uniform self,
                         const varying vec3i &sampleID,
                         const varying DifferentialGeometry &dg,
                         const varying vec3f &shadingNormal)
{
  const int accumID = sampleID.z;
  const int ix = sampleID.x;
  const int iy = sampleID.y;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, (self->super.fb->size.x * iy) + ix, accumID);

  float occlusion = 0.f;
  const linear3f localToWorld = frame(shadingNormal);

  for (uniform int i = 0; i < self->aoSamples; i++) {
    const vec2f s = RandomTEA__getFloats(rng);
    const vec3f local_ao_dir = cosineSampleHemisphere(s);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    if (dot(ao_dir, dg.Ns) < 0.05f) { // check below surface
      occlusion += 1.f;
      continue;
    }

    Ray ao_ray;
    setRay(ao_ray, dg.P + (self->super.epsilon * dg.Ns), ao_dir,
           self->super.epsilon, self->aoDistance);

    if (self->aoTransparencyEnabled) {
      const float rayOffset = self->super.epsilon*(1.f + s.x);
      occlusion += (1.f - lightAlpha(self, ao_ray, self->super.model, 1.0f,
            rayOffset, sampleID, 0.2f));
    } else if (isOccluded(self->super.model, ao_ray))
      occlusion += 1.f;
  }

  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir) cancel
  return 1.0f - occlusion/self->aoSamples;
}

inline void shadeAO(const uniform SciVisRenderer *uniform self,
                    const varying vec3i &sampleID,
                    const varying DifferentialGeometry &dg,
                    const varying SciVisShadingInfo &info,
                    varying vec3f &color)
{
  // Calculate AO contribution as ambient term
  float ao = 1.0f;
  if (self->aoSamples > 0 && reduce_max(self->aoColor) > 0.f) {
    if (self->aoDistance > 0.f)
      ao *= calculateAO(self, sampleID, dg, info.shadingNormal);
    // Blend AO w/ diffuse term
  }
  color = color + (info.local_opacity * ao) * info.Kd * self->aoColor;
}

inline void shadeLights(const uniform SciVisRenderer *uniform self,
                        const varying Ray &ray,
                        const varying DifferentialGeometry &dg,
                        const varying SciVisShadingInfo &info,
                        varying vec3f &color,
                        const varying float &rayOffset,
                        const varying vec3i &sampleID,
                        const uniform float quality)
{
  const vec3f R = ray.dir - ((2.f * dot(ray.dir, info.shadingNormal)) * info.shadingNormal);
  const vec3f P = dg.P + self->super.epsilon * dg.Ng;

  //calculate shading for all lights
  for (uniform int i = 0; self->lights && i < self->numLights; i++) {
    const uniform Light *uniform l = self->lights[i];
    const vec2f s = make_vec2f(0.0f); // sample center of area lights
    const Light_SampleRes light = l->sample(l, dg, s);

    if (reduce_max(light.weight) > 0.f) { // any potential contribution?
      float cosNL = dot(light.dir, info.shadingNormal);
      if (self->oneSidedLighting) {
        if (cosNL < 0.0f)
          continue;
      }
      else
        cosNL = abs(cosNL);
      const float cosLR = max(0.f, dot(light.dir, R));
      vec3f specular = info.Ks * powf(cosLR, info.Ns);
      const vec3f brdf = info.Kd * cosNL + specular;
      const vec3f light_contrib = info.local_opacity
                                  * brdf * light.weight;

      if (self->shadowsEnabled) 
      {
        const float max_contrib = reduce_max(light_contrib);
        if (max_contrib > .01f) {
          Ray shadowRay;
          setRay(shadowRay, P, light.dir);
          const float light_alpha = lightAlpha(self, shadowRay,
                                               self->super.model,
                                               max_contrib,
                                               rayOffset, sampleID, quality);
          color = color + light_alpha * light_contrib;
        }
      } else {
        color = color + light_contrib;
      }
    }
  }
}

// Volume shading //

inline vec4f SciVisRenderer_computeVolumeSample(SciVisRenderer *uniform renderer,
                                               Volume *uniform volume,
                                               varying Ray &ray, const varying float &rayOffset,
                                               const varying vec3i &sampleID)
{
  // Sample the volume at the hit point in world coordinates.
  const vec3f coordinates = ray.org + ray.t0 * ray.dir;
  const float sample = volume->computeSample(volume, coordinates);

  // Look up the color associated with the volume sample.
  vec3f sampleColor =
      volume->transferFunction->getColorForValue(volume->transferFunction,
                                                 sample);

  // Look up the opacity associated with the volume sample.
  const float sampleOpacity =
      volume->transferFunction->getOpacityForValue(volume->transferFunction,
                                                   sample);

  // Compute gradient shading, if enabled.
  if(volume->gradientShadingEnabled) {

    // Use volume gradient as the normal.
    const vec3f gradient =
        safe_normalize(volume->computeGradient(volume, coordinates));

    // Setup differential geometry for the volume sample point.
    DifferentialGeometry dg;
    dg.P = coordinates;
    dg.Ns = gradient;

    // Compute lighting.
    vec3f shadedColor = make_vec3f(0.f);
    const vec2f s = make_vec2f(0.0f); // sample center of area lights

    for (uniform uint32 i=0; i<renderer->numLights; i++) {
      const Light_SampleRes light =
          renderer->lights[i]->sample(renderer->lights[i], dg, s);
      const float cosNL = (gradient.x == 0.f
                           && gradient.y == 0.f
                           && gradient.z == 0.f) ?
                          1.f : abs(dot(safe_normalize(light.dir),
                                        gradient));

      shadedColor = shadedColor + sampleColor * cosNL * light.weight * one_over_pi /*BRDF normalization to match surface shading*/;
    }

   //sampleColor = shadedColor;

    dg.color = make_vec4f(sampleColor, 1);
    dg.Ng = dg.Ns;
    SciVisShadingInfo info;
    initShadingInfo(info);

    //hardcode volume spec for now
    info.Ks = make_vec3f(.3f,.3f,.3f);
    info.Ns = 20.0f;
    info.local_opacity=1.f;
    dg.material = 0;
    shadeMaterials(dg, info);

    vec3f litColor = make_vec3f(0.0f);
    info.Ns *= 0.3f;
    shadeLights(renderer, ray, dg, info, litColor, rayOffset, sampleID,0.5f);
    sampleColor = litColor;
  }

  // return the color contribution for this sample only (do not accumulate)
  return clamp(sampleOpacity / volume->samplingRate)
          * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);
}

// Volume shading //
// quality is from 0 - 1.  1 being best.
vec4f SciVisRenderer_computeVolumeInterval(const SciVisRenderer *uniform renderer,
                                               Volume *uniform volume,
                                               varying Ray &ray, float tBegin,
                                               float tEnd, float maxOpacity, bool isShadowRay, const varying float &rayOffset, const varying vec3i &sampleID, uniform float quality)
{
  vec3f singleCoordinates;
  float singleMax = -1.f;
  float singleMin = 0.01f;
  ray.t0 = tBegin;
  float tSkipped = -1;  //for adaptive, skip adapting sampling rate up to this value
  vec4f intervalColor = make_vec4f(0.f);
  volume->intersect(volume, ray);
  // const uniform float oneOverQuality = (1.f/quality);
  float samplingRate = volume->samplingRate*quality;
  float lastSample = -1.f;
  const float adaptiveScalar = volume->adaptiveScalar*quality;
  const float maxSamplingRate = volume->adaptiveMaxSamplingRate*quality; //2.f nyquist frequency
  const float adaptiveBacktrack = volume->adaptiveBacktrack;
  while (ray.t0 < tEnd && intervalColor.w < maxOpacity)
  {
    // Sample the volume at the hit point in world coordinates.
    const vec3f coordinates = ray.org + ray.t0 * ray.dir;
    const float sample = volume->computeSample(volume, coordinates);
    if (lastSample == -1.f)
      lastSample = sample;

    // Look up the color associated with the volume sample.
    // Look up the opacity associated with the volume sample.
    vec3f sampleColor;
    float sampleOpacity;
    sampleOpacity = volume->transferFunction->getIntegratedOpacityForValue(volume->transferFunction,
                                                    lastSample, sample);
    if (volume->adaptiveSampling && sampleOpacity > adaptiveBacktrack && ray.t0 > tSkipped)  // adaptive backtack
    {
      //adaptively refine sampling rate
      const float newSamplingRate = min(adaptiveScalar*sampleOpacity*quality,maxSamplingRate);
      if (newSamplingRate > samplingRate + 0.01f)
      {
        samplingRate = newSamplingRate;
        const float ot0 = ray.t0;
        ray.t0 = max(max(ray.t0-max(ray.time,volume->samplingStep),tSkipped), tBegin);
        //Carson: TODO: skipping past ray.time shouldnt be necessary and avoided 
        //but currently greatly reduces artifacts from low to high opacity 
        //regions.  issue with space skipping?
        tSkipped = ot0;
        volume->intersectAdaptive(volume, ray, samplingRate);
        lastSample = -1;
        continue;
      }
      tSkipped = ray.t0;
    }

    if (!isShadowRay)
    {
      sampleColor = volume->transferFunction->getIntegratedColorForValue(volume->transferFunction,  lastSample,  sample);
      lastSample = sample;

      // Compute gradient shading, if enabled.
      if(volume->gradientShadingEnabled) 
      {
        if (volume->singleShade && sampleOpacity*(1.0f - intervalColor.w) > singleMax && sampleOpacity > singleMin)
        {
          singleMax = sampleOpacity*(1.0f - intervalColor.w);
          singleCoordinates = coordinates;
        }
        else if (!volume->singleShade)
        {
          // Use volume gradient as the normal.
          const vec3f gradient =
              safe_normalize(volume->computeGradient(volume, coordinates));

          // Setup differential geometry for the volume sample point.
          DifferentialGeometry dg;
          dg.Ng = gradient;
          if (dot(ray.dir,dg.Ng) >= 0.f) dg.Ng = neg(dg.Ng); //face forward
          dg.Ns = dg.Ng;
          dg.P = coordinates + dg.Ns*renderer->volumeEpsilon;

          dg.color = make_vec4f(sampleColor, 1);
          dg.Ng = dg.Ns;
          SciVisShadingInfo info;
          initShadingInfo(info);

          //hardcode volume spec for now
          info.Ks = volume->specular*sampleOpacity;
          info.Ns = 20.0f;
          info.local_opacity=sampleOpacity;
          dg.material = 0;
          shadeMaterials(dg, info);

          vec3f litColor = make_vec3f(0.0f);
          info.Ns *= 0.3f;
          shadeLights(renderer, ray, dg, info, litColor, rayOffset, sampleID, quality*0.5f);
          sampleColor = litColor;
        }
      }
    }
    if (volume->adaptiveSampling)
    {
      // ray.time = absolute step length.  convert to effective sampling rate
      float dt = 1.0f/((ray.time)/volume->samplingStep);
      vec4f contribution =  clamp(sampleOpacity / dt)
          * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);
      intervalColor = intervalColor + (1.0f - intervalColor.w) * contribution;
      //adaptively refine sampling rate
      if (ray.t0 > tSkipped)
        samplingRate = max(min(adaptiveScalar*sampleOpacity*quality,maxSamplingRate)/*nyquist frequency*/,volume->samplingRate*quality);
      volume->intersectAdaptive(volume, ray, samplingRate);
    }
    else
    {
      vec4f contribution =  clamp(sampleOpacity / volume->samplingRate)
          * make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);
      intervalColor = intervalColor + (1.0f - intervalColor.w) * contribution;
      volume->intersect(volume, ray);
    }
  }
  if(!isShadowRay && volume->gradientShadingEnabled && volume->singleShade && singleMax >= singleMin) 
  {
    // Use volume gradient as the normal.
    const vec3f gradient =
        safe_normalize(volume->computeGradient(volume, singleCoordinates));

    // Setup differential geometry for the volume sample point.
    DifferentialGeometry dg;
    dg.Ng = gradient;  
    if (dot(ray.dir,dg.Ng) >= 0.f) dg.Ng = neg(dg.Ng); //face forward
    dg.Ns = dg.Ng;
    dg.P = singleCoordinates + dg.Ns*renderer->volumeEpsilon;

    // Compute lighting.
    vec3f shadedColor = make_vec3f(0.f);

    dg.color = intervalColor;
    SciVisShadingInfo info;
    initShadingInfo(info);

    //hardcode volume spec for now
    info.Ks = volume->specular*singleMax;
    info.Ns = 30.0f;
    info.local_opacity=intervalColor.w;
    dg.material = 0;
    shadeMaterials(dg, info);

    vec3f litColor = make_vec3f(0.0f);
    shadeAO(renderer, sampleID, dg, info, litColor);
    info.Ns *= 0.3f;
    shadeLights(renderer, ray, dg, info, litColor, rayOffset, sampleID, quality*0.5f);
    intervalColor = make_vec4f(litColor,intervalColor.w);
  }
  return intervalColor;
}

inline
vec4f SciVisRenderer_computeGeometrySample(SciVisRenderer *uniform self,
                                          const varying vec3i &sampleID,
                                          varying Ray &ray, const varying float &rayOffset)
{
  vec3f color        = make_vec3f(0.f);
  float path_opacity = 1.f;
  uniform int path_depth = 0;
  float first_hit_dist = 0.f;

  while (1) {

    traceRay(self->super.model, ray);

    // Check if we missed, if so we are done //
    if (ray.geomID < 0) {
      return make_vec4f(color, 1.f - path_opacity);
    }

    // Record depth of first hit for depth output //
    if (path_depth == 0) {
      first_hit_dist = ray.t;
    }

    // Start shading //

    // Calculate material information from DG //
    DifferentialGeometry dg;
    postIntersect(self->super.model,
                  dg,
                  ray,
                  DG_NG|DG_NS|DG_NORMALIZE|DG_FACEFORWARD|DG_TANGENTS|
                  DG_MATERIALID|DG_COLOR|DG_TEXCOORD);

    SciVisShadingInfo info;
    initShadingInfo(info);

    shadeMaterials(dg, info);

    info.local_opacity = path_opacity * info.d;

    if (info.local_opacity > 0.01f) { // worth shading?
      shadeAO(self, sampleID, dg, info, color);
      shadeLights(self, ray, dg, info, color, rayOffset,sampleID, 0.5f);
    }

    // Kill path when reached max depth or if remaining contribution too low
    path_opacity = path_opacity * (1.f - info.d);
    path_depth++;
    if (path_depth >= self->maxDepth || path_opacity < 0.01f ) {
      ray.t = first_hit_dist;
      return make_vec4f(color, 1.f - path_opacity);
    }

    // Reset ray
    ray.t0 = ray.t + self->super.epsilon;
    ray.t = infinity;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

/*! Returns the first hit volume for the provided ray and sets the ray bounds
 *  t0 and t, considering the provided ray offset and any clipping. If no
 *  volume is found, the returned volume is NULL and ray.t0 will be set to
 *  infinity.
 */
inline Volume *
SciVisRenderer_intersectVolumes(const uniform SciVisRenderer *uniform renderer,
                                varying Ray &ray,
                                const varying float &rayOffset)
{
  // The first intersected volume.
  Volume * volume = NULL;

  // The ray with bounds for the first intersected volume.
  Ray volumeRay = ray;
  volumeRay.t0 = infinity;

  // Test each volume and find the first intersection.
  for (uniform int32 i=0; i<renderer->super.model->volumeCount; i++) {
    Volume *uniform volume_i = renderer->super.model->volumes[i];

    // Intersect volume bounding box.
    float t0, t1;
    intersectBox(ray, volume_i->boundingBox, t0, t1);

    // Clip against volume clipping box (if specified).
    if(ne(volume_i->volumeClippingBox.lower,
          volume_i->volumeClippingBox.upper)) {
      float tClip0, tClip1;
      intersectBox(ray, volume_i->volumeClippingBox, tClip0, tClip1);

      t0 = max(t0, tClip0);
      t1 = min(t1, tClip1);
    }

    // Update intersected volume.
    if (t0 < t1 && t0 < volumeRay.t0) {
      volumeRay.t0 = t0;
      volumeRay.t = t1;
      volume = volume_i;
    }
  }

  // Update the provided ray.
  ray = volumeRay;

  // If we intersected a volume, offset ray by a fraction of the nominal ray
  // step.
  if (volume)
    ray.t0 += rayOffset * volume->samplingStep * rcpf(volume->samplingRate);

  // Return the first intersected volume.
  return volume;
}

// Lighting functions //

inline float lightAlpha(const uniform SciVisRenderer *uniform self,
                        Ray &ray, uniform Model *uniform model,
                        const float weight,
                        const varying float &rayOffset,
                        const varying vec3i &sampleID,
                        const uniform float quality)
{
  float alpha = 1.f;
  const float org_t_max = ray.t;
  uniform int remaining_depth = self->maxDepth;

  while (1) {
    // Get first intersected volume for each ray and set the ray bounds.
    float org_t = ray.t;
    float org_t0 = ray.t0;
    Volume *volume = SciVisRenderer_intersectVolumes(self, ray, rayOffset);
    float volumeT = ray.t;
    float volumeT0 = ray.t0;
    ray.t = org_t;
    ray.t0 = org_t0;

    traceRay(model,ray);

    if (!(volumeT0 <= 0.f || volumeT0 <= ray.t))
      volume = NULL;

    if (!volume && ray.geomID < 0) return alpha;

    float material_opacity = 1.f;

    if (!volume)
    {
      DifferentialGeometry dg;
      postIntersect(model, dg, ray, DG_MATERIALID | DG_TEXCOORD | DG_COLOR);

      uniform SciVisMaterial *scivisMaterial =
          (uniform SciVisMaterial *)dg.material;

      if(scivisMaterial == NULL) {
        material_opacity = dg.color.w;
      } else {
        foreach_unique( mat in scivisMaterial ) {
          material_opacity = mat->d * get1f(mat->map_d, dg.st, 1.f);
          if (mat->map_Kd) {
            vec4f Kd_from_map = get4f(mat->map_Kd,dg.st);
            material_opacity *= Kd_from_map.w;
          }
        }
      }
    }
    else
    {
      ray.t = org_t;
      ray.t0 = org_t0;
      // print("found volume intersection!\n");
      // Provide ray offset for use with isosurface geometries (this value
      // ignored elsewhere).
      ray.time = -rayOffset * volume->samplingStep;
      ray.t = volumeT;
      float tBegin = max(volumeT0,0.f);
      float tEnd = volumeT+self->volumeEpsilon;
      foreach_unique (v in volume)
      {
        vec4f volumeColor = SciVisRenderer_computeVolumeInterval(self, v, ray, tBegin, tEnd, 0.98f, 1, rayOffset, sampleID, quality);
        material_opacity = volumeColor.w;
      }  
    }

    alpha = alpha * (1.f - material_opacity);

    if (alpha * weight < ALPHA_THRESHOLD) return alpha;

    if (--remaining_depth <= 0)
      return alpha;

    ray.t0 = ray.t + rayOffset;
    ray.t = org_t_max;
    ray.primID = -1;
    ray.geomID = -1;
    ray.instID = -1;
  }
}

/*! This function intersects the volume and geometries. */
void SciVisRenderer_intersect(uniform SciVisRenderer *uniform renderer,
                              varying Ray &ray,
                              const varying float &rayOffset,
                              const varying vec3i &sampleID,
                              varying vec4f &color,
                              varying float &depth)
{
  // Original tMax for ray interval
  const float tMax = ray.t;

  // Copy of the ray for geometry intersection. The original ray is
  // used for volume intersection.
  Ray geometryRay = ray;
  geometryRay.primID = -1;
  geometryRay.geomID = -1;
  geometryRay.instID = -1;

  // Get first intersected volume for each ray and set the ray bounds.
  Volume *volume = SciVisRenderer_intersectVolumes(renderer, ray, rayOffset);

  // Provide ray offset for use with isosurface geometries (this value
  // ignored elsewhere).
  if (volume != NULL)
    geometryRay.time = -rayOffset * volume->samplingStep;

  // Initial trace through geometries.
  vec4f geometryColor = SciVisRenderer_computeGeometrySample(renderer,
      sampleID, geometryRay, rayOffset);

  // Depth is the first volume bounding box or geometry hit
  depth = min(ray.t0, geometryRay.t);

  // Trace the ray through the volume and geometries.
  float firstHit;

  if(geometryRay.t < ray.t0 || volume == NULL) {
    // Geometry contribution.
    color = geometryColor;
  }

  while ((firstHit = min(ray.t0, geometryRay.t)) < tMax
         /*&& min(min(color.x, color.y), color.z) < 1.0f*///<--why is this here?
         && color.w < 0.99f) {

    if (firstHit == ray.t0) {

      // Check to see if we've exited the current volume.
      if (ray.t0 >= ray.t) {

        ray.t0 = ray.t + renderer->volumeEpsilon;
        ray.t = tMax;
        volume = SciVisRenderer_intersectVolumes(renderer, ray, rayOffset);

      } else {

        if (any(volume == NULL))
          print("ACCESSING NULL VOLUME HERE!\n");

        // Compute the volume sample at the current position and advance the ray
        vec4f volumeColor;
        float tBegin = ray.t0;
        float tEnd = min(geometryRay.t,ray.t);
        foreach_unique (v in volume)
        {
          //volumeColor = SciVisRenderer_computeVolumeSample(renderer, v, ray);
          // Advance the ray for the next sample.
          //volume->intersect(v, ray);

          //interval sampling
          volumeColor = SciVisRenderer_computeVolumeInterval(renderer, v, ray, tBegin, tEnd, .98f - color.w, 0, rayOffset, sampleID, 1.f);
        }

        // Volume contribution.
        color = color + (1.0f - color.w) * volumeColor;
        ray.t0 = tEnd + renderer->volumeEpsilon;
      }

    } else {// firstHit == geometryRay.t

      // Geometry contribution.
      color = color + (1.0f - color.w) * geometryColor;

      if (color.w < 0.99f) {
        // Reset geometry ray.
        geometryRay.t0 = geometryRay.t + renderer->volumeEpsilon;
        geometryRay.t = tMax; //!< end of valid ray interval for traceRay()
        geometryRay.primID = -1;
        geometryRay.geomID = -1;
        geometryRay.instID = -1;

        // Update ray offset for use with isosurface geometries based on current
        // volume (this value ignored elsewhere).
        geometryRay.time = volume ? -rayOffset * volume->samplingStep : 0.f;

        // Trace next geometry ray.
        geometryColor = SciVisRenderer_computeGeometrySample(renderer,
            sampleID, geometryRay, rayOffset);
      }

    }
  }
}

void SciVisRenderer_renderSample(uniform Renderer *uniform _self,
                                 void *uniform perFrameData,
                                 varying ScreenSample &sample)
{
  SciVisRenderer *uniform renderer
    = (SciVisRenderer *uniform) _self;

  // Ray offset for this sample, as a fraction of the nominal step size.
  float rayOffset = precomputedHalton2(sample.sampleID.z);
  int ix = sample.sampleID.x % 4;
  int iy = sample.sampleID.y % 4;
  int patternID = ix + 4 * iy;
  rayOffset += precomputedHalton3(patternID);

  if(rayOffset > 1.f) rayOffset -= 1.f;

  // Provide the renderer to the intersector as it contains all
  // volumes, geometries, etc.
  vec4f color = make_vec4f(0.0f);
  float depth = infinity;

  SciVisRenderer_intersect(renderer, sample.ray, rayOffset,
                           sample.sampleID, color, depth);

  // blend with background
  if (renderer->super.backgroundEnabled) {
    color = color + (1.0f - color.w) * make_vec4f(renderer->super.bgColor, 0.f);
  }

  // Store the result in the sample.
  sample.rgb = make_vec3f(color);
  sample.alpha = color.w;
  sample.z = depth;
}

static unmasked void *uniform
SciVisRenderer_beginFrame(uniform Renderer *uniform _self,
                          uniform FrameBuffer *uniform fb)
{
  uniform SciVisRenderer *uniform self = (uniform SciVisRenderer *uniform)_self;
  self->super.fb = fb;
  Model *uniform model = self->super.model;
  self->volumeEpsilon = self->super.epsilon;

  if (model->volumeCount) {
    // Ray epsilon based on bounding box of all volumes.
    uniform box3f boundingBox = make_box3f_empty();
    for (uniform int32 i = 0; i < model->volumeCount; i++)
      boundingBox = box_extend(boundingBox, model->volumes[i]->boundingBox);

    self->volumeEpsilon = 1e-3f * length(box_size(boundingBox));
  }

  return NULL;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void SciVisRenderer_set(void *uniform _self,
                               const uniform bool shadowsEnabled,
                               const uniform int maxDepth,
                               const uniform int aoSamples,
                               const uniform float aoDistance,
                               const uniform vec3f &aoColor,
                               const uniform bool aoTransparencyEnabled,
                               void **uniform lights,
                               const uniform uint32 numLights,
                               const uniform bool oneSidedLighting
                               )
{
  uniform SciVisRenderer *uniform self = (uniform SciVisRenderer *uniform)_self;

  self->shadowsEnabled = shadowsEnabled;
  self->maxDepth = maxDepth;
  self->aoSamples = aoSamples;
  self->aoDistance = aoDistance;
  self->aoTransparencyEnabled = aoTransparencyEnabled;

  // already factor in parts of cosineSampleHemispherePDF
  self->aoColor = aoColor * pi;

  self->lights = (const uniform Light *uniform *uniform)lights;
  self->numLights = numLights;
  self->oneSidedLighting = oneSidedLighting;
}

export void *uniform SciVisRenderer_create(void *uniform cppE)
{
  uniform SciVisRenderer *uniform self = uniform new uniform SciVisRenderer;
  Renderer_Constructor(&self->super,cppE);
  self->super.renderSample = SciVisRenderer_renderSample;
  self->super.beginFrame = SciVisRenderer_beginFrame;
  SciVisRenderer_set(self, false, 10, 4, infinity, make_vec3f(0.25f), false, NULL, 0, true);

  return self;
}
