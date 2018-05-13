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

static inline uniform vec3f tetNormal(UnstructuredVolume *uniform self,
                                      uniform uint64 id,
                                      uniform int planeID)
{
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  const int *uniform idx = (const int *uniform)&self->indices[2 * id + 1];
  const vec3f *uniform vtx = self->vertices;

  const uniform int planes[4][3] = {{1, 2, 3}, {2, 0, 3}, {3, 0, 1}, {0, 2, 1}};
  const int *uniform plane = planes[planeID];

  return normalize(cross(vtx[idx[plane[1]]] - vtx[idx[plane[0]]],
                         vtx[idx[plane[2]]] - vtx[idx[plane[0]]]));
}

static inline uniform vec3f hexNormal(UnstructuredVolume *uniform self,
                                      uniform uint64 id,
                                      uniform int planeID)
{
  if (self->faceNormals)
    return self->faceNormals[(id * 6) + planeID];

  const int *uniform idx = (const int *uniform)&self->indices[2 * id];
  const vec3f *uniform vtx = self->vertices;

  const uniform int planes[6][3] = {{0, 2, 1}, {0, 5, 4}, {0, 7, 3},
                                    {6, 5, 1}, {6, 7, 4}, {6, 2, 3}};
  const int *uniform plane = planes[planeID];

  return normalize(cross(vtx[idx[plane[1]]] - vtx[idx[plane[0]]],
                         vtx[idx[plane[2]]] - vtx[idx[plane[0]]]));
}

bool intersectAndSampleTet(void *uniform userData,
                           uniform uint64 id,
                           uniform bool assumeInside,
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

  uniform vec3f norm0 = tetNormal(self, id, 0);
  uniform vec3f norm1 = tetNormal(self, id, 1);
  uniform vec3f norm2 = tetNormal(self, id, 2);
  uniform vec3f norm3 = tetNormal(self, id, 3);

  // Distance from the world point to the faces.
  float d0 = dot(norm0, p1 - samplePos);
  float d1 = dot(norm1, p2 - samplePos);
  float d2 = dot(norm2, p3 - samplePos);
  float d3 = dot(norm3, p0 - samplePos);

  if (!assumeInside && !(d0 > 0 && d1 > 0 && d2 > 0 && d3 > 0))
    return false;

  if (self->cellField) {
    result = self->cellField[id];
    return true;
  }

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
void WedgeInterpolationFunctions(float pcoords[3], float sf[6])
{
  sf[0] = (1.0 - pcoords[0] - pcoords[1]) * (1.0 - pcoords[2]);
  sf[1] = pcoords[0] * (1.0 - pcoords[2]);
  sf[2] = pcoords[1] * (1.0 - pcoords[2]);
  sf[3] = (1.0 - pcoords[0] - pcoords[1]) * pcoords[2];
  sf[4] = pcoords[0] * pcoords[2];
  sf[5] = pcoords[1] * pcoords[2];
}

//----------------------------------------------------------------------------
void WedgeInterpolationDerivs(float pcoords[3], float derivs[18])
{
  // r-derivatives
  derivs[0] = -1.0 + pcoords[2];
  derivs[1] =  1.0 - pcoords[2];
  derivs[2] =  0.0;
  derivs[3] = -pcoords[2];
  derivs[4] =  pcoords[2];
  derivs[5] =  0.0;

  // s-derivatives
  derivs[6] = -1.0 + pcoords[2];
  derivs[7] =  0.0;
  derivs[8] =  1.0 - pcoords[2];
  derivs[9] = -pcoords[2];
  derivs[10] = 0.0;
  derivs[11] = pcoords[2];

  // t-derivatives
  derivs[12] = -1.0 + pcoords[0] + pcoords[1];
  derivs[13] = -pcoords[0];
  derivs[14] = -pcoords[1];
  derivs[15] =  1.0 - pcoords[0] - pcoords[1];
  derivs[16] =  pcoords[0];
  derivs[17] =  pcoords[1];
}

static const float WEDGE_DIVERGED = 1.e6;
static const int WEDGE_MAX_ITERATION = 10;
static const float WEDGE_CONVERGED = 1.e-05;
static const float WEDGE_OUTSIDE_CELL_TOLERANCE = 1.e-06;

bool intersectAndSampleWedge(void *uniform userData,
                             uniform uint64 id,
                             uniform bool assumeInside,
                             float &result,
                             vec3f samplePos,
                             float range_lo,
                             float range_hi)
{
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) userData;

  float params[3] = { 0.5, 0.5, 0.5 };
  float pcoords[3] = { 0.5, 0.5, 0.5 };
  float derivs[18];
  float weights[6];

  memset(derivs, 0, sizeof(derivs));
  memset(weights, 0, sizeof(weights));

  const int *uniform idx = (const int *uniform)&self->indices[2 * id] + 2;

  uniform const int edges[9][2] = { {0,1}, {1,2}, {2,0},
                                    {3,4}, {4,5}, {5,3},
                                    {0,3}, {1,4}, {2,5} };
  uniform float longestEdge = 0;
  for (uniform int i = 0; i < 9; i++) {
      uniform vec3f p0 = self->vertices[idx[edges[i][0]]];
      uniform vec3f p1 = self->vertices[idx[edges[i][1]]];
      uniform float dist = distance(p0, p1);
      if (longestEdge < dist)
         longestEdge = dist;
  }

  uniform float volumeBound = pow(longestEdge, 3);
  uniform float determinantTolerance =
      1e-20 < .00001*volumeBound ? 1e-20 : .00001*volumeBound;

  //  enter iteration loop
  bool converged = false;
  for (uniform int iteration = 0; !converged && (iteration < WEDGE_MAX_ITERATION); iteration++) {
    //  calculate element interpolation functions and derivatives
    WedgeInterpolationFunctions(pcoords, weights);
    WedgeInterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    vec3f fcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f rcol = make_vec3f(0.f, 0.f, 0.f);
    vec3f scol = make_vec3f(0.f, 0.f, 0.f);
    vec3f tcol = make_vec3f(0.f, 0.f, 0.f);
    for (uniform int i = 0; i < 6; i++) {
      vec3f pt = self->vertices[idx[i]];
      fcol = fcol + pt * weights[i];
      rcol = rcol + pt * derivs[i];
      scol = scol + pt * derivs[i + 6];
      tcol = tcol + pt * derivs[i + 12];
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
    if (((absf(pcoords[0] - params[0])) < WEDGE_CONVERGED) &&
        ((absf(pcoords[1] - params[1])) < WEDGE_CONVERGED) &&
        ((absf(pcoords[2] - params[2])) < WEDGE_CONVERGED)) {
      converged = true;
    } else if ((absf(pcoords[0]) > WEDGE_DIVERGED) ||
               (absf(pcoords[1]) > WEDGE_DIVERGED) ||
               (absf(pcoords[2]) > WEDGE_DIVERGED)) {
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

  float lowerlimit = 0.0 - WEDGE_OUTSIDE_CELL_TOLERANCE;
  float upperlimit = 1.0 + WEDGE_OUTSIDE_CELL_TOLERANCE;
  if (assumeInside || (pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
                       pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
                       pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit &&
                       pcoords[0] + pcoords[1] <= upperlimit)) {
    // evaluation
    if (self->cellField) {
      result = self->cellField[id];
    } else {
      result = 0.f;
      WedgeInterpolationFunctions(pcoords, weights);
      for (int i = 0; i < 6; i++) {
        result += weights[i] * self->field[idx[i]];
      }
    }

    return true;
  }

  return false;
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
    if (self->cellField) {
      result = self->cellField[id];
    } else {
      result = 0.f;
      HexInterpolationFunctions(pcoords, weights);
      for (int i = 0; i < 8; i++) {
        int idx = i < 4 ? (&lower.x)[i] : (&upper.x)[i - 4];
        result += weights[i] * self->field[idx];
      }
    }

    return true;
  }

  return false;
}

bool intersectAndSampleHexPlanar(void *uniform userData,
                                 uniform uint64 id,
                                 uniform bool assumeInside,
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
          hexNormal(self, id, planeID));
    if (!assumeInside && dist[planeID] >= 0.f)
      return false;
  }

  if (self->cellField) {
    result = self->cellField[id];
    return true;
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
    intersectAndSampleTet(userData, id, false, result, samplePos, range_lo, range_hi);
  } else if (self->indices[2 * id].x == -2) {
    intersectAndSampleWedge(userData, id, false, result, samplePos, range_lo, range_hi);
  } else {
    if (self->hexMethod == PLANAR)
      intersectAndSampleHexPlanar(userData, id, false, result, samplePos, range_lo, range_hi);
    else if (self->hexMethod == NONPLANAR)
      intersectAndSampleHexNonplanar(userData, id, result, samplePos, range_lo, range_hi);
  }
}

inline varying float UnstructuredVolume_sample(
    void *uniform _self, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;

  float results = floatbits(0xffffffff);  /* NaN */

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

inline bool sameSign(float a, float b)
{
  return a * b >= 0.f;
}

void UnstructuredVolume_intersectIsoRay(void *uniform userData,
                                        uniform float *uniform isovalues,
                                        uniform int numIsovalues,
                                        uniform size_t geomID,
                                        uniform uint64 id,
                                        varying Ray &ray)
{
  //  print("intersectIsoRay\n");
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) userData;

  if (self->indices[2 * id].x == -1) {
    // tetrahedron

    uniform vec4i t = self->indices[2 * id + 1];  // The 4 corner indices of the tetrahedron.

    float t_nr = ray.t0;
    float t_fr = ray.t;
    for (uniform int plane = 0; plane < 4; plane ++) {
      const uniform vec3f p0 = self->vertices[(&t.x)[(plane + 1) & 0x3]];
      const uniform vec3f norm = tetNormal(self, id, plane);

      const float side_dot = dot(ray.dir, norm);
      const float side_t = rcp(side_dot) * dot(p0 - ray.org, norm);

      if (side_dot < 0.f) {
        t_nr = max(t_nr, side_t);
      } else {
        t_fr = min(t_fr, side_t);
      }
    }

    if (t_nr > t_fr)
      return;

    float t0 = t_nr; float t1 = t_fr;
    float p0, p1;
    intersectAndSampleTet(userData, id, true, p0, ray.org + ray.dir * t0, 0.f, 0.f);
    intersectAndSampleTet(userData, id, true, p1, ray.org + ray.dir * t1, 0.f, 0.f);

    for (uniform int i = 0; i < numIsovalues; i++) {
      const uniform float piso = isovalues[i];
      if (sameSign(p0 - piso, p1 - piso))
        return;

      const float t_hit = t0 + (t1 - t0) * ((piso - p0) / (p1 - p0));
      if (t_hit < ray.t) {
        ray.t = t_hit;
        ray.geomID = geomID;
        ray.primID = i;
        return;
      }
    }

    return;
  } else if (self->indices[2 * id].x == -2) {
    // wedge

    const uniform int faces[8][3] = {{0, 2, 1}, {3, 4, 5},
                                     {0, 3, 2}, {1, 4, 0}, {2, 5, 1},
                                     {0, 4, 3}, {1, 5, 4}, {2, 3, 5}};

    const int *uniform idx = (const int *uniform)&self->indices[2 * id] + 2;
    float t_nr = pos_inf;
    float t_fr = neg_inf;

    // Moller-Trumobore ray-triangle intersection
    const uniform float EPSILON = 0.0000001;
    for (uniform int face = 0; face < 8; face++) {
      uniform vec3f v0 = self->vertices[idx[faces[face][0]]];
      uniform vec3f v1 = self->vertices[idx[faces[face][1]]];
      uniform vec3f v2 = self->vertices[idx[faces[face][2]]];

      uniform vec3f edge1 = v1 - v0;
      uniform vec3f edge2 = v2 - v0;
      vec3f h = cross(ray.dir, edge2);
      float a = dot(edge1, h);
      if (a > -EPSILON && a < EPSILON)
        continue;
      float f = 1/a;
      vec3f s = ray.org - v0;
      float u = f * dot(s, h);
      if (u < 0.f || u > 1.f)
        continue;
      vec3f q = cross(s, edge1);
      float v = f * dot(ray.dir, q);
      if (v < 0.f || u + v > 1.f)
        continue;
      float t = f * dot(edge2, q);
      if (t > EPSILON) {
        t_nr = min(t_nr, t);
        t_fr = max(t_fr, t);
      } else {
        continue;
      }
    }

    if (t_nr >= t_fr)
      return;

      //Neubauer method
#define NUM_ITERATIONS 4

    for (uniform iso = 0; iso < numIsovalues; iso++) {
      float t0 = t_nr; float t1 = t_fr;
      float p0, p1;
      intersectAndSampleWedge(userData, id, true, p0, ray.org + ray.dir * t0, 0.f, 0.f);
      intersectAndSampleWedge(userData, id, true, p1, ray.org + ray.dir * t1, 0.f, 0.f);

      const uniform float piso = isovalues[iso];
      if (sameSign(p0 - piso, p1 - piso))
        return;

      for(uniform int i = 0; i < NUM_ITERATIONS; i++) {
        const float t = t0 + (t1 - t0) * ((piso - p0) / (p1 - p0));
        float pt;
        intersectAndSampleWedge(userData, id, true, pt, ray.org + ray.dir * t, 0.f, 0.f);

        if (sameSign(pt - piso, p0 - piso)) {
          t0 = t;
          p0 = pt;
        } else {
          t1 = t;
          p1 = pt;
        }
      }
      const float t_hit = t0 + (t1 - t0) * ((piso - p0) / (p1 - p0));
      if (t_hit < ray.t) {
        ray.t = t_hit;
        ray.geomID = geomID;
        ray.primID = iso;
        return;
      }
    }
  } else {
    // hexahedron

    uniform vec4i lower = self->indices[2 * id];
    uniform vec4i upper = self->indices[2 * id + 1];

    float t_nr = ray.t0;
    float t_fr = ray.t;
    for (uniform int plane = 0; plane < 6; plane ++) {
      const uniform vec3f p0 = self->vertices[plane < 3 ? lower.x : upper.z];
      const uniform vec3f norm = hexNormal(self, id, plane);

      const float side_dot = dot(ray.dir, norm);
      const float side_t = rcp(side_dot) * dot(p0 - ray.org, norm);

      if (side_dot < 0.f) {
        t_nr = max(t_nr, side_t);
      } else {
        t_fr = min(t_fr, side_t);
      }
    }

    if (t_nr > t_fr)
      return;

    //Neubauer method
#define NUM_ITERATIONS 4

    for (uniform iso = 0; iso < numIsovalues; iso++) {
      float t0 = t_nr; float t1 = t_fr;
      float p0, p1;
      intersectAndSampleHexPlanar(userData, id, true, p0, ray.org + ray.dir * t0, 0.f, 0.f);
      intersectAndSampleHexPlanar(userData, id, true, p1, ray.org + ray.dir * t1, 0.f, 0.f);

      const uniform float piso = isovalues[iso];
      if (sameSign(p0 - piso, p1 - piso))
        return;

      for(uniform int i = 0; i < NUM_ITERATIONS; i++) {
        const float t = t0 + (t1 - t0) * ((piso - p0) / (p1 - p0));
        float pt;
        intersectAndSampleHexPlanar(userData, id, true, pt, ray.org + ray.dir * t, 0.f, 0.f);

        if (sameSign(pt - piso, p0 - piso)) {
          t0 = t;
          p0 = pt;
        } else {
          t1 = t;
          p1 = pt;
        }
      }
      const float t_hit = t0 + (t1 - t0) * ((piso - p0) / (p1 - p0));
      if (t_hit < ray.t) {
        ray.t = t_hit;
        ray.geomID = geomID;
        ray.primID = iso;
        return;
      }
    }
  }
}

inline void UnstructuredVolume_intersectIsosurface(void *uniform _self,
                                                   uniform float *uniform isovalues,
                                                   uniform int numIsovalues,
                                                   uniform size_t geomID,
                                                   varying Ray &ray)
{
  // Cast to the actual Volume subtype.
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;

  uniform vec2f isoRange = make_vec2f(isovalues[0], isovalues[0]);

  for (uniform int i = 1; i < numIsovalues; i++) {
    isoRange.x = min(isoRange.x, isovalues[i]);
    isoRange.y = max(isoRange.y, isovalues[i]);
  }

  traverseRay(self->bvh, _self, isoRange,
              UnstructuredVolume_intersectIsoRay,
              isovalues, numIsovalues, geomID,
              ray);
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

export void
UnstructuredVolume_disableCellGradient(void *uniform _self)
{
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;

  if (self->cellField)
    self->super.gradientShadingEnabled = false;
}

export void
UnstructuredVolume_setFaceNormals(void *uniform _self,
                                  const vec3f *uniform _faceNormals)
{
  UnstructuredVolume *uniform self = (UnstructuredVolume * uniform) _self;

  self->faceNormals = _faceNormals;
}

export void UnstructuredVolume_set(void *uniform _self,
                                  const uniform int &_nVertices,
                                  const uniform int &_nCells,
                                  const uniform box3f &_bbox,
                                  const vec3f *uniform _vertices,
                                  const vec4i *uniform _indices,
                                  const float *uniform _field,
                                  const float *uniform _cellField,
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
  self->cellField   = _cellField;

  // Set inherited member variables.
  self->super.boundingBox  = _bbox;
  self->super.samplingRate = samplingRate;
  self->super.samplingStep = samplingStep;

  self->faceNormals = NULL;

  self->bvh.rootRef = rootRef;
  self->bvh.node    = (MinMaxBVH2Node * uniform) _bvhNode;
  self->bvh.primID  = _bvhPrimID;
}
