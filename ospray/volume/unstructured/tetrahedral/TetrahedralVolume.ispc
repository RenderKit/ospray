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

#include "TetrahedralVolume.ih"

bool intersectAndSampleTet(void *uniform userData,
                           uniform uint64 id,
                           float &result,
                           vec3f samplePos,
                           float range_lo,
                           float range_hi)
{
  TetrahedralVolume *uniform self = (TetrahedralVolume * uniform) userData;

  vec4i t = self->tetrahedra[id];  // The 4 corner indices of the tetrahedron.

  vec3f p0 = self->vertices[t.x];
  vec3f p1 = self->vertices[t.y];
  vec3f p2 = self->vertices[t.z];
  vec3f p3 = self->vertices[t.w];

  uniform vec3f norm0 = self->faceNormals[(id * 4) + 0];
  uniform vec3f norm1 = self->faceNormals[(id * 4) + 1];
  uniform vec3f norm2 = self->faceNormals[(id * 4) + 2];
  uniform vec3f norm3 = self->faceNormals[(id * 4) + 3];

  // Distance from the world point to the faces.
  float d0 = dot(norm0, p1 - samplePos);
  float d1 = dot(norm1, p2 - samplePos);
  float d2 = dot(norm2, p3 - samplePos);
  float d3 = dot(norm3, p0 - samplePos);

  if (!(d0 > 0 && d1 > 0 && d2 > 0 && d3 > 0))
    return false;

  // Distance of tetrahedron corners to their opposite faces.
  float h0 = dot(norm0, p1 - p0);
  float h1 = dot(norm1, p2 - p1);
  float h2 = dot(norm2, p3 - p2);
  float h3 = dot(norm3, p0 - p3);

  // Local coordinates = ratio of distances.
  float z0 = d0 / h0;
  float z1 = d1 / h1;
  float z2 = d2 / h2;
  float z3 = d3 / h3;

  // Field/attribute values at the tetrahedron corners.
  float v0 = self->field[t.x];
  float v1 = self->field[t.y];
  float v2 = self->field[t.z];
  float v3 = self->field[t.w];

  // Interpolated field/attribute value at the world position.
  result = z0 * v0 + z1 * v1 + z2 * v2 + z3 * v3;

  return true;
}

inline varying float TetrahedralVolume_sample(
    void *uniform _self, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  TetrahedralVolume *uniform self = (TetrahedralVolume * uniform) _self;

  float results = 0;

  traverse(self->bvh, _self, intersectAndSampleTet, results, worldCoordinates);

  return results;
}

inline varying vec3f TetrahedralVolume_computeGradient(
    void *uniform _self, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  TetrahedralVolume *uniform self = (TetrahedralVolume * uniform) _self;

  // Gradient step in each dimension (world coordinates).
  const uniform vec3f gradientStep = make_vec3f(self->super.samplingStep);

  // The gradient will be computed using central differences.
  varying vec3f gradient;

  // Forward differences.

  // Sample at gradient location.
  float sample = self->super.sample(self, worldCoordinates);

  // Gradient magnitude in the X direction.
  gradient.x =
      self->super.sample(
          self, worldCoordinates + make_vec3f(gradientStep.x, 0.0f, 0.0f)) -
      sample;

  // Gradient magnitude in the Y direction.
  gradient.y =
      self->super.sample(
          self, worldCoordinates + make_vec3f(0.0f, gradientStep.y, 0.0f)) -
      sample;

  // Gradient magnitude in the Z direction.
  gradient.z =
      self->super.sample(
          self, worldCoordinates + make_vec3f(0.0f, 0.0f, gradientStep.z)) -
      sample;

  // This approximation may yield image artifacts.
  return gradient / gradientStep;
}

// ray.time is set to interval length of intersected sample
inline void TetrahedralVolume_stepRay(
    void *uniform _self, varying Ray &ray, const varying float samplingRate)
{
  // Cast to the actual Volume subtype.
  TetrahedralVolume *uniform self = (TetrahedralVolume * uniform) _self;

  // The recommended step size for ray casting based volume renderers.
  const varying float step = self->super.samplingStep / samplingRate;

  ray.t0 += step;
  ray.time = step;

  ray.geomID = 1;
  ray.primID = 1;
  ray.instID = 1;
}

inline void TetrahedralVolume_intersectIsosurface(void *uniform _self,
                                                  uniform float *uniform
                                                      isovalues,
                                                  uniform int numIsovalues,
                                                  varying Ray &ray)
{
  // Cast to the actual Volume subtype.
  TetrahedralVolume *uniform self = (TetrahedralVolume * uniform) _self;

  // The nominal step size for ray casting based volume renderers, not
  // considering the sampling rate.
  const uniform float step = self->super.samplingStep;

  // Tentatively advance the ray.
  ray.t0 += step;
  if (ray.t0 >= ray.t)
    return;
}

void TetrahedralVolume_Constructor(TetrahedralVolume *uniform self,
                                   void *uniform cppEquivalent)
{
  Volume_Constructor(&self->super, cppEquivalent);

  // Set the ispc functions.
  self->super.sample              = TetrahedralVolume_sample;
  self->super.computeGradient     = TetrahedralVolume_computeGradient;
  self->super.stepRay             = TetrahedralVolume_stepRay;
  self->super.intersectIsosurface = TetrahedralVolume_intersectIsosurface;
}

export void *uniform
TetrahedralVolume_createInstance(void *uniform cppEquivalent)
{
  TetrahedralVolume *uniform self = uniform new uniform TetrahedralVolume;

  TetrahedralVolume_Constructor(self, cppEquivalent);

  return self;
}

export void TetrahedralVolume_set(void *uniform _self,
                                  const uniform int &_nVertices,
                                  const uniform int &_nTetrahedra,
                                  const uniform box3f &_bbox,
                                  const vec3f *uniform _vertices,
                                  const vec3f *uniform _faceNormals,
                                  const vec4i *uniform _tetrahedra,
                                  const float *uniform _field,
                                  uniform int64 rootRef,
                                  const void *uniform _bvhNode,
                                  const int64 *uniform _bvhPrimID,
                                  const uniform float samplingRate,
                                  const uniform float samplingStep)
{
  uniform TetrahedralVolume *uniform self =
      (uniform TetrahedralVolume * uniform) _self;

  self->nVertices   = _nVertices;
  self->nTetrahedra = _nTetrahedra;
  self->vertices    = _vertices;
  self->tetrahedra  = _tetrahedra;
  self->field       = _field;

  // Set inherited member variables.
  self->super.boundingBox  = _bbox;
  self->super.samplingRate = samplingRate;
  self->super.samplingStep = samplingStep;

  self->faceNormals = _faceNormals;

  self->bvh.rootRef = rootRef;
  self->bvh.node    = (MinMaxBVH2Node * uniform) _bvhNode;
  self->bvh.primID  = _bvhPrimID;
}
