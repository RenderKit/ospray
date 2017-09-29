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

#include "SciVisRenderer.ih"
#include "surfaceShading.ih"

vec4f SciVisRenderer_computeVolumeSample(SciVisRenderer *uniform renderer,
                                         Volume *uniform volume,
                                         varying Ray &ray,
                                         const varying float &rayOffset,
                                         const varying vec3i &sampleID)
{
  // Sample the volume at the hit point in world coordinates.
  const vec3f coordinates = ray.org + ray.t0 * ray.dir;
  const float sample      = volume->sample(volume, coordinates);

  // Look up the color associated with the volume sample.
  vec3f sampleColor = volume->transferFunction->getColorForValue(
      volume->transferFunction, sample);

  // Look up the opacity associated with the volume sample.
  const float sampleOpacity = volume->transferFunction->getOpacityForValue(
      volume->transferFunction, sample);

  // Compute gradient shading, if enabled.
  if (volume->gradientShadingEnabled) {
    // Use volume gradient as the normal.
    const vec3f gradient =
        safe_normalize(volume->computeGradient(volume, coordinates));

    // Setup differential geometry for the volume sample point.
    DifferentialGeometry dg;
    dg.P  = coordinates;
    dg.Ns = gradient;

    // Compute lighting.
    vec3f shadedColor = make_vec3f(0.f);
    const vec2f s     = make_vec2f(0.0f);  // sample center of area lights

    for (uniform uint32 i = 0; i < renderer->numLights; i++) {
      const Light_SampleRes light =
          renderer->lights[i]->sample(renderer->lights[i], dg, s);
      const float cosNL =
          (gradient.x == 0.f && gradient.y == 0.f && gradient.z == 0.f)
              ? 1.f
              : abs(dot(safe_normalize(light.dir), gradient));

      shadedColor =
          shadedColor +
          sampleColor * cosNL * light.weight *
              one_over_pi /*BRDF normalization to match surface shading*/;
    }

    dg.color = make_vec4f(sampleColor, 1);
    dg.Ng    = dg.Ns;
    SciVisShadingInfo info;
    initShadingInfo(info);

    // hardcode volume spec for now
    info.Ks            = make_vec3f(.3f, .3f, .3f);
    info.Ns            = 20.0f;
    info.local_opacity = 1.f;
    dg.material        = 0;
    shadeMaterials(dg, info);

    vec3f litColor = make_vec3f(0.0f);
    info.Ns *= 0.3f;
    integrateOverLights(
        renderer, ray, dg, info, litColor, rayOffset, sampleID, 0.5f);
    sampleColor = litColor;
  }

  // return the color contribution for this sample only (do not accumulate)
  return clamp(sampleOpacity / volume->samplingRate) *
         make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);
}

// Volume shading //
// quality is from 0 - 1.  1 being best.
vec4f SciVisRenderer_computeVolumeInterval(const SciVisRenderer *uniform
                                               renderer,
                                           Volume *uniform volume,
                                           varying Ray &ray,
                                           float tBegin,
                                           float tEnd,
                                           float maxOpacity,
                                           bool isShadowRay,
                                           const varying float &rayOffset,
                                           const varying vec3i &sampleID,
                                           uniform float quality)
{
  // ray to model coordinate
  //  rayTransform(ray, volume->rcp_xfm);
  // start computation
  vec3f singleCoordinates;
  float singleMax            = -1.f;
  float singleMin            = 0.01f;
  float volumeSamplingRate   = volume->samplingRate;
  float samplingRate         = volumeSamplingRate * quality;
  float lastSample           = -1.f;
  const float adaptiveScalar = volume->adaptiveScalar * quality;
  const float maxSamplingRate =
      volume->adaptiveMaxSamplingRate * quality;  // 2.f nyquist frequency
  const float adaptiveBacktrack = volume->adaptiveBacktrack;
  ray.t0                        = tBegin;
  // ray.time = ray.t0-tBegin;
  float tSkipped =
      -1f;  // for adaptive, skip adapting sampling rate up to this value
  vec4f intervalColor = make_vec4f(0.f);

  // TODO: initially sampling by max samplingRate produced artifacts, not sure
  // why.
  if (volume->adaptiveSampling)
    volume->stepRay(volume, ray, samplingRate);
  else
    volume->stepRay(volume, ray, volumeSamplingRate);

  tBegin = tBegin + renderer->volumeEpsilon;
  while (ray.t0 < tEnd && intervalColor.w < maxOpacity) {
    // Sample the volume at the hit point in world coordinates.
    const vec3f coordinates = ray.org + ray.t0 * ray.dir;
    const float sample      = volume->sample(volume, coordinates);
    if (lastSample == -1.f)
      lastSample = sample;

    // Look up the color associated with the volume sample.
    // Look up the opacity associated with the volume sample.
    vec3f sampleColor;
    float sampleOpacity;
    sampleOpacity = volume->transferFunction->getIntegratedOpacityForValue(
        volume->transferFunction, lastSample, sample);
    if (volume->adaptiveSampling && sampleOpacity > adaptiveBacktrack &&
        ray.t0 > tSkipped)  // adaptive backtack
    {
      // adaptively refine sampling rate
      const float newSamplingRate =
          min(adaptiveScalar * sampleOpacity * quality, maxSamplingRate);
      if (newSamplingRate > samplingRate + 0.01f) {
        samplingRate    = newSamplingRate;
        const float ot0 = ray.t0;
        ray.t0 =
            max(max(ray.t0 - max(ray.time, volume->samplingStep), tSkipped),
                tBegin);
        // Carson: TODO: skipping past ray.time shouldnt be necessary and
        // avoided
        // but currently greatly reduces artifacts from low to high opacity
        // regions.  issue with space skipping?
        tSkipped = ot0;
        volume->stepRay(volume, ray, samplingRate);
        lastSample = -1;
        continue;
      }
      tSkipped = ray.t0;
    }

    if (!isShadowRay) {
      sampleColor = volume->transferFunction->getIntegratedColorForValue(
          volume->transferFunction, lastSample, sample);
      lastSample = sample;

      // Compute gradient shading, if enabled.
      if (volume->gradientShadingEnabled) {
        if (volume->singleShade &&
            sampleOpacity * (1.0f - intervalColor.w) > singleMax &&
            sampleOpacity > singleMin) {
          singleMax         = sampleOpacity * (1.0f - intervalColor.w);
          singleCoordinates = coordinates;
        } else if (!volume->singleShade) {
          // Use volume gradient as the normal.
          const vec3f gradient =
              safe_normalize(volume->computeGradient(volume, coordinates));

          // Setup differential geometry for the volume sample point.
          DifferentialGeometry dg;
          dg.Ng = gradient;
          if (dot(ray.dir, dg.Ng) >= 0.f)
            dg.Ng = neg(dg.Ng);  // face forward
          dg.Ns   = dg.Ng;
          dg.P    = coordinates + dg.Ns * renderer->volumeEpsilon;

          dg.color = make_vec4f(sampleColor, 1);
          dg.Ng    = dg.Ns;
          SciVisShadingInfo info;
          initShadingInfo(info);

          // hardcode volume spec for now
          info.Ks            = volume->specular;
          info.Ns            = volume->ns;
          info.local_opacity = 1.f;
          dg.material        = 0;
          shadeMaterials(dg, info);

          vec3f litColor = make_vec3f(0.0f);
          shadeAO(renderer, sampleID, dg, info, litColor);
          integrateOverLights(renderer,
                              ray,
                              dg,
                              info,
                              litColor,
                              rayOffset,
                              sampleID,
                              quality * 0.5f);
          sampleColor = litColor;
        }
      }
    }
    if (volume->adaptiveSampling) {
      // ray.time = absolute step length.  convert to effective sampling rate
      float dt = 1.0f / ((ray.time) / volume->samplingStep);
      vec4f contribution =
          clamp(sampleOpacity / dt) *
          make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);
      intervalColor = intervalColor + (1.0f - intervalColor.w) * contribution;
      // adaptively refine sampling rate
      if (ray.t0 > tSkipped)
        samplingRate = max(min(adaptiveScalar * sampleOpacity * quality,
                               maxSamplingRate) /*nyquist frequency*/,
                           volume->samplingRate * quality);
      volume->stepRay(volume, ray, samplingRate);
    } else {
      vec4f contribution =
          clamp(sampleOpacity / volume->samplingRate) *
          make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.0f);
      intervalColor = intervalColor + (1.0f - intervalColor.w) * contribution;
      volume->stepRay(volume, ray, volumeSamplingRate);
    }
  }
  if (!isShadowRay && volume->gradientShadingEnabled && volume->singleShade &&
      singleMax >= singleMin) {
    // Use volume gradient as the normal.
    const vec3f gradient =
        safe_normalize(volume->computeGradient(volume, singleCoordinates));

    // Setup differential geometry for the volume sample point.
    DifferentialGeometry dg;
    dg.Ng = gradient;
    if (dot(ray.dir, dg.Ng) >= 0.f)
      dg.Ng = neg(dg.Ng);  // face forward
    dg.Ns   = dg.Ng;
    dg.P    = singleCoordinates + dg.Ns * renderer->volumeEpsilon;

    // Compute lighting.
    vec3f shadedColor = make_vec3f(0.f);

    dg.color = intervalColor;
    SciVisShadingInfo info;
    initShadingInfo(info);

    // hardcode volume spec for now
    info.Ks            = volume->specular * singleMax;
    info.Ns            = volume->ns;
    info.local_opacity = intervalColor.w;
    dg.material        = 0;
    shadeMaterials(dg, info);

    vec3f litColor = make_vec3f(0.0f);
    shadeAO(renderer, sampleID, dg, info, litColor);
    integrateOverLights(
        renderer, ray, dg, info, litColor, rayOffset, sampleID, quality * 0.5f);
    intervalColor = make_vec4f(litColor, intervalColor.w);
  }
  // ray back to world coordinate
  //  rayTransform(ray, volume->xfm);
  return intervalColor;
}

/*! Returns the first hit volume for the provided ray and sets the ray bounds
 *  t0 and t, considering the provided ray offset and any clipping. If no
 *  volume is found, the returned volume is NULL and ray.t0 will be set to
 *  infinity.
 */
Volume *SciVisRenderer_intersectVolumes(const uniform SciVisRenderer *uniform
                                            renderer,
                                        varying Ray &ray,
                                        const varying float &rayOffset)
{
  // The first intersected volume.
  Volume *volume = NULL;

  // The ray with bounds for the first intersected volume.
  Ray volumeRay = ray;
  volumeRay.t0  = infinity;

  // Test each volume and find the first intersection.
  for (uniform int32 i = 0; i < renderer->super.model->volumeCount; i++) {
    Volume *uniform volume_i = renderer->super.model->volumes[i];

    // ray to model coordinate
    //    rayTransform(ray, volume_i->rcp_xfm);

    // Intersect volume bounding box.
    float t0, t1;
    intersectBox(ray, volume_i->boundingBox, t0, t1);

    // Clip against volume clipping box (if specified).
    if (ne(volume_i->volumeClippingBox.lower,
           volume_i->volumeClippingBox.upper)) {
      float tClip0, tClip1;
      intersectBox(ray, volume_i->volumeClippingBox, tClip0, tClip1);

      t0 = max(t0, tClip0);
      t1 = min(t1, tClip1);
    }

    // ray back to world coordinate
    //    rayTransform(ray, volume_i->xfm);

    // Update intersected volume.
    if (t0 < t1 && t0 < volumeRay.t0) {
      volumeRay.t0 = t0;
      volumeRay.t  = t1;
      volume       = volume_i;
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
