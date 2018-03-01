// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "UnstructuredVolume.ih"

bool intersectAndSampleTet(void *uniform userData,
                           uniform uint64 id,
                           float &result,
                           vec3f samplePos,
                           float range_lo,
                           float range_hi)
{
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) userData;

  uniform vec4i t = self->indices[2 * id + 1];  // The 4 corner indices of the tetrahedron.

  uniform vec3f p0 = self->vertices[t.x];
  uniform vec3f p1 = self->vertices[t.y];
  uniform vec3f p2 = self->vertices[t.z];
  uniform vec3f p3 = self->vertices[t.w];

  uniform vec3f norm0 = self->faceNormals[(id * 6) + 0];
  uniform vec3f norm1 = self->faceNormals[(id * 6) + 1];
  uniform vec3f norm2 = self->faceNormals[(id * 6) + 2];
  uniform vec3f norm3 = self->faceNormals[(id * 6) + 3];

  // Distance from the world point to the faces.
  float d0 = dot(norm0, p1 - samplePos);
  float d1 = dot(norm1, p2 - samplePos);
  float d2 = dot(norm2, p3 - samplePos);
  float d3 = dot(norm3, p0 - samplePos);

  if (!(d0 > 0 && d1 > 0 && d2 > 0 && d3 > 0))
    return false;

  // Distance of tetrahedron corners to their opposite faces.
  uniform float h0 = dot(norm0, p1 - p0);
  uniform float h1 = dot(norm1, p2 - p1);
  uniform float h2 = dot(norm2, p3 - p2);
  uniform float h3 = dot(norm3, p0 - p3);

  // Local coordinates = ratio of distances.
  float z0 = d0 / h0;
  float z1 = d1 / h1;
  float z2 = d2 / h2;
  float z3 = d3 / h3;

  // Field/attribute values at the tetrahedron corners.
  uniform float v0 = self->field[t.x];
  uniform float v1 = self->field[t.y];
  uniform float v2 = self->field[t.z];
  uniform float v3 = self->field[t.w];

  // Interpolated field/attribute value at the world position.
  result = z0 * v0 + z1 * v1 + z2 * v2 + z3 * v3;

  return true;
}

//----------------------------------------------------------------------------
// Compute iso-parametric interpolation functions
//
void HexInterpolationFunctions(float pcoords[3], float sf[8])
{
  float rm, sm, tm;

  rm = 1.f - pcoords[0];
  sm = 1.f - pcoords[1];
  tm = 1.f - pcoords[2];

  sf[0] = rm * sm * tm;
  sf[1] = pcoords[0] * sm * tm;
  sf[2] = pcoords[0] *pcoords[1] * tm;
  sf[3] = rm * pcoords[1] * tm;
  sf[4] = rm * sm * pcoords[2];
  sf[5] = pcoords[0] * sm * pcoords[2];
  sf[6] = pcoords[0] * pcoords[1] * pcoords[2];
  sf[7] = rm * pcoords[1] * pcoords[2];
}

//----------------------------------------------------------------------------
void HexInterpolationDerivs(float pcoords[3], float derivs[24])
{
  float rm, sm, tm;

  rm = 1.f - pcoords[0];
  sm = 1.f - pcoords[1];
  tm = 1.f - pcoords[2];

  // r-derivatives
  derivs[0] = -sm * tm;
  derivs[1] = sm * tm;
  derivs[2] = pcoords[1] * tm;
  derivs[3] = -pcoords[1] * tm;
  derivs[4] = -sm * pcoords[2];
  derivs[5] = sm * pcoords[2];
  derivs[6] = pcoords[1] * pcoords[2];
  derivs[7] = -pcoords[1] * pcoords[2];

  // s-derivatives
  derivs[8] = -rm * tm;
  derivs[9] = -pcoords[0] * tm;
  derivs[10] = pcoords[0] * tm;
  derivs[11] = rm * tm;
  derivs[12] = -rm * pcoords[2];
  derivs[13] = -pcoords[0] * pcoords[2];
  derivs[14] = pcoords[0] * pcoords[2];
  derivs[15] = rm * pcoords[2];

  // t-derivatives
  derivs[16] = -rm * sm;
  derivs[17] = -pcoords[0] * sm;
  derivs[18] = -pcoords[0] * pcoords[1];
  derivs[19] = -rm * pcoords[1];
  derivs[20] = rm * sm;
  derivs[21] = pcoords[0] * sm;
  derivs[22] = pcoords[0] * pcoords[1];
  derivs[23] = rm * pcoords[1];
}

static const float HEX_DIVERGED = 1.e6;
static const int HEX_MAX_ITERATION = 10;
static const float HEX_CONVERGED = 1.e-05;
static const float HEX_OUTSIDE_CELL_TOLERANCE = 1.e-06;

bool intersectAndSampleHexNonplanar(void *uniform userData,
                                    uniform uint64 id,
                                    float &result,
                                    vec3f samplePos,
                                    float range_lo,
                                    float range_hi)
{
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) userData;

  float params[3] = { 0.5, 0.5, 0.5 };
  float pcoords[3];
  float derivs[24];
  float weights[8];

  uniform vec4i lower = self->indices[2 * id];
  uniform vec4i upper = self->indices[2 * id + 1];

  // should precompute these
  uniform const int diagonals[4][2] = { { 0, 2}, { 1, 3}, {2, 0}, {3, 1} };
  uniform float longestDiagonal = 0;
  for (uniform int i = 0; i < 4; i++) {
      uniform vec3f p0 = self->vertices[(&lower.x)[diagonals[i][0]]];
      uniform vec3f p1 = self->vertices[(&upper.x)[diagonals[i][1]]];
      uniform float dist = distance(p0, p1);
      if (longestDiagonal < dist)
         longestDiagonal = dist;
  }

  uniform float volumeBound = pow(longestDiagonal, 3);
  uniform float determinantTolerance =
      1e-20 < .00001*volumeBound ? 1e-20 : .00001*volumeBound;

  //  set initial position for Newton's method
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;

  //  enter iteration loop
  bool converged = false;
  for (uniform int iteration = 0; !converged && (iteration < HEX_MAX_ITERATION); iteration++) {
    //  calculate element interpolation functions and derivatives
    HexInterpolationFunctions(pcoords, weights);
    HexInterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    vec3f fcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f rcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f scol = make_vec3f(0.f, 0.f, 0.f);
    vec3f tcol = make_vec3f(0.f, 0.f, 0.f);
    for (uniform int i = 0; i < 8; i++) {
      vec3f pt =
        i < 4 ? self->vertices[(&lower.x)[i]] : self->vertices[(&upper.x)[i - 4]];

      fcol = fcol + pt * weights[i];
      rcol = rcol + pt * derivs[i];
      scol = scol + pt * derivs[i + 8];
      tcol = tcol + pt * derivs[i + 16];
    }

    fcol = fcol - samplePos;

    // compute determinants and generate improvements
    float d = det(make_LinearSpace3f(rcol, scol, tcol));
    if (absf(d) < determinantTolerance) {
      return false;
    }

    pcoords[0] = params[0] - det(make_LinearSpace3f(fcol, scol, tcol)) / d;
    pcoords[1] = params[1] - det(make_LinearSpace3f(rcol, fcol, tcol)) / d;
    pcoords[2] = params[2] - det(make_LinearSpace3f(rcol, scol, fcol)) / d;

    // convergence/divergence test - if neither, repeat
    if (((absf(pcoords[0] - params[0])) < HEX_CONVERGED) &&
        ((absf(pcoords[1] - params[1])) < HEX_CONVERGED) &&
        ((absf(pcoords[2] - params[2])) < HEX_CONVERGED)) {
      converged = true;
    } else if ((absf(pcoords[0]) > HEX_DIVERGED) ||
               (absf(pcoords[1]) > HEX_DIVERGED) ||
               (absf(pcoords[2]) > HEX_DIVERGED)) {
      return false;
    } else {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
    }
  }

  if (!converged) {
    return false;
  }

  float lowerlimit = 0.0 - HEX_OUTSIDE_CELL_TOLERANCE;
  float upperlimit = 1.0 + HEX_OUTSIDE_CELL_TOLERANCE;
  if (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
      pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
      pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit) {
    // evaluation
    result = 0.f;
    HexInterpolationFunctions(pcoords, weights);
    for (int i = 0; i < 8; i++) {
      int idx = i < 4 ? (&lower.x)[i] : (&upper.x)[i - 4];
      result += weights[i] * self->field[idx];
    }

    return true;
  }

  return false;
}

bool intersectAndSampleHexPlanar(void *uniform userData,
                                 uniform uint64 id,
                                 float &result,
                                 vec3f samplePos,
                                 float range_lo,
                                 float range_hi)
{
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) userData;

  uniform vec4i lower = self->indices[2 * id];
  uniform vec4i upper = self->indices[2 * id + 1];

  float dist[6];
  for (uniform int planeID = 0; planeID < 6; planeID++) {
    dist[planeID] =
      dot(samplePos - self->vertices[planeID < 3 ? lower.x : upper.z],
          self->faceNormals[(id * 6) + planeID]);
    if (dist[planeID] >= 0.f)
      return false;
  }

  float u0 = dist[4] / (dist[0] + dist[4]);
  float v0 = dist[5] / (dist[1] + dist[5]);
  float w0 = dist[3] / (dist[2] + dist[3]);
  float u1 = 1.f - u0;
  float v1 = 1.f - v0;
  float w1 = 1.f - w0;
  result =
    u0 * v0 * w0 * self->field[lower.x] +
    u0 * v0 * w1 * self->field[lower.y] +
    u0 * v1 * w1 * self->field[lower.z] +
    u0 * v1 * w0 * self->field[lower.w] +
    u1 * v0 * w0 * self->field[upper.x] +
    u1 * v0 * w1 * self->field[upper.y] +
    u1 * v1 * w1 * self->field[upper.z] +
    u1 * v1 * w0 * self->field[upper.w];

  return true;
}

bool intersectAndSampleCell(void *uniform userData,
                           uniform uint64 id,
                           float &result,
                           vec3f samplePos,
                           float range_lo,
                           float range_hi)
{
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) userData;

  if (self->indices[2 * id].x == -1) {
    intersectAndSampleTet(userData, id, result, samplePos, range_lo, range_hi);
  } else {
    if (self->hexMethod == PLANAR)
      intersectAndSampleHexPlanar(userData, id, result, samplePos, range_lo, range_hi);
    else if (self->hexMethod == NONPLANAR)
      intersectAndSampleHexNonplanar(userData, id, result, samplePos, range_lo, range_hi);
  }
}

inline varying float UnstructuredVolume_sample(
    void *uniform _self, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;

  float results = 0;

  traverse(self->bvh, _self, intersectAndSampleCell, results, worldCoordinates);

  return results;
}

inline varying vec3f UnstructuredVolume_computeGradient(
    void *uniform _self, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;

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
inline void UnstructuredVolume_stepRay(
    void *uniform _self, varying Ray &ray, const varying float samplingRate)
{
  // Cast to the actual Volume subtype.
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;

  // The recommended step size for ray casting based volume renderers.
  const varying float step = self->super.samplingStep / samplingRate;

  ray.t0 += step;
  ray.time = step;

  ray.geomID = 1;
  ray.primID = 1;
  ray.instID = 1;
}

inline void UnstructuredVolume_intersectIsosurface(void *uniform _self,
                                                  uniform float *uniform
                                                      isovalues,
                                                  uniform int numIsovalues,
                                                  varying Ray &ray)
{
  // Cast to the actual Volume subtype.
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;

  // The nominal step size for ray casting based volume renderers, not
  // considering the sampling rate.
  const uniform float step = self->super.samplingStep;

  // Tentatively advance the ray.
  ray.t0 += step;
  if (ray.t0 >= ray.t)
    return;
}

void UnstructuredVolume_Constructor(UnstructuredVolume *uniform self,
                                   void *uniform cppEquivalent)
{
  Volume_Constructor(&self->super, cppEquivalent);

  // Set the ispc functions.
  self->super.sample              = UnstructuredVolume_sample;
  self->super.computeGradient     = UnstructuredVolume_computeGradient;
  self->super.stepRay             = UnstructuredVolume_stepRay;
  self->super.intersectIsosurface = UnstructuredVolume_intersectIsosurface;
}

export void *uniform
UnstructuredVolume_createInstance(void *uniform cppEquivalent)
{
  UnstructuredVolume *uniform self = uniform new uniform UnstructuredVolume;

  UnstructuredVolume_Constructor(self, cppEquivalent);

  return self;
}

export void
UnstructuredVolume_method_planar(void *uniform _self)
{
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;
  self->hexMethod = PLANAR;
}

export void
UnstructuredVolume_method_nonplanar(void *uniform _self)
{
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;
  self->hexMethod = NONPLANAR;
}

export void UnstructuredVolume_set(void *uniform _self,
                                  const uniform int &_nVertices,
                                  const uniform int &_nCells,
                                  const uniform box3f &_bbox,
                                  const vec3f *uniform _vertices,
                                  const vec3f *uniform _faceNormals,
                                  const vec4i *uniform _indices,
                                  const float *uniform _field,
                                  uniform int64 rootRef,
                                  const void *uniform _bvhNode,
                                  const int64 *uniform _bvhPrimID,
                                  const uniform float samplingRate,
                                  const uniform float samplingStep)
{
  uniform UnstructuredVolume *uniform self =
      (uniform UnstructuredVolume * uniform) _self;

  self->nVertices   = _nVertices;
  self->nCells      = _nCells;
  self->vertices    = _vertices;
  self->indices     = _indices;
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
