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

#include <embree2/rtcore.isph>
#include <embree2/rtcore_ray.isph>
#ifndef RTCORE_VERSION_MAJOR
#include <embree2/rtcore_version.h>
#endif

extern "C" uniform float callSample(void *uniform volume,
                                    uniform float world_x,
                                    uniform float world_y,
                                    uniform float world_z);

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

  uniform vec3f faceNormals[4] = self->faceNormals[id].normals;
  uniform vec3f norm0          = faceNormals[0];
  uniform vec3f norm1          = faceNormals[1];
  uniform vec3f norm2          = faceNormals[2];
  uniform vec3f norm3          = faceNormals[3];

  // Distance from the world point to the faces.
  float d0 = dot(norm0, p1 - samplePos);
  float d1 = dot(norm1, p2 - samplePos);
  float d2 = dot(norm2, p3 - samplePos);
  float d3 = dot(norm3, p0 - samplePos);

  if (!(d0 > 0 && d1 > 0 && d2 > 0 && d3 > 0)) {
    return false;
  }

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

  // The sum of the location coordinates should add up to approximately 1.
  float z_total = z0 + z1 + z2 + z3;
  // assert((0.9) < z_total && z_total < (1.1));

  // Field/attribute values at the tetrahedron corners.
  float v0 = self->field[t.x];
  float v1 = self->field[t.y];
  float v2 = self->field[t.z];
  float v3 = self->field[t.w];

  // Interpolated field/attribute value at the world position.
  result = z0 * v0 + z1 * v1 + z2 * v2 + z3 * v3;

  return true;
}

inline varying float TetrahedralVolume_computeSample(
    void *uniform _self, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  TetrahedralVolume *uniform self = (TetrahedralVolume * uniform) _self;

  // print("TetrahedralVolume_computeSample\n");

  float results = 0;

  // return 0.5f;

  traverse(self->bvh, _self, intersectAndSampleTet, results, worldCoordinates);

#if (0)  // use ispc

  RTCRay ray;
  ray.dirx   = 1;
  ray.diry   = 0;
  ray.dirz   = 0;
  ray.orgx   = worldCoordinates.x;
  ray.orgy   = worldCoordinates.y;
  ray.orgz   = worldCoordinates.z;
  ray.tnear  = 0;
  ray.tfar   = 1.0e8;
  ray.primID = -1;
  ray.geomID = -1;

  rtcIntersect(self->embree_scene, ray);

  if (ray.primID < self->nTetrahedra) {
    int id = ray.primID;  // varying

    // print("ispc embree found an intersection %\n\n", i);

    vec3f world =
        make_vec3f(worldCoordinates.x, worldCoordinates.y, worldCoordinates.z);

    vec4i t = self->tetrahedra[id];  // The 4 corner indices of the tetrahedron.

    vec3f p0 = self->vertices[t.x];
    vec3f p1 = self->vertices[t.y];
    vec3f p2 = self->vertices[t.z];
    vec3f p3 = self->vertices[t.w];

    vec3f faceNormals[4] = self->faceNormals[id].normals;
    vec3f norm0          = faceNormals[0];
    vec3f norm1          = faceNormals[1];
    vec3f norm2          = faceNormals[2];
    vec3f norm3          = faceNormals[3];

    // Distance from the world point to the faces.
    float d0 = dot(norm0, p1 - world);
    float d1 = dot(norm1, p2 - world);
    float d2 = dot(norm2, p3 - world);
    float d3 = dot(norm3, p0 - world);

    // redo intersection test to validate ???

    // Interpolate the field value at the world position using the values at the
    // corners.

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

    // The sum of the location coordinates should add up to approximately 1.
    // float z_total = z0 + z1 + z2 + z3;
    // assert((0.9) < z_total && z_total < (1.1));

    // Field/attribute values at the tetrahedron corners.
    float v0 = self->field[t.x];
    float v1 = self->field[t.y];
    float v2 = self->field[t.z];
    float v3 = self->field[t.w];

    // Interpolated field/attribute value at the world position.
    results = (z0 * v0 + z1 * v1 + z2 * v2 + z3 * v3);
  }
#endif

#if 0  // use cpp

  foreach_active(i) {
    uniform float x_coord = extract(worldCoordinates.x,i);
    uniform float y_coord = extract(worldCoordinates.y,i);
    uniform float z_coord = extract(worldCoordinates.z,i);
    uniform float r = callSample(self->super.cppEquivalent, x_coord, y_coord, z_coord);
    results = insert(results, i, r);
  }

#endif

  return results;
}

inline varying vec3f TetrahedralVolume_computeGradient(
    void *uniform _self, const varying vec3f &worldCoordinates)
{
  // Cast to the actual Volume subtype.
  TetrahedralVolume *uniform self = (TetrahedralVolume * uniform) _self;

  // The gradient will be computed using central differences.
  varying vec3f gradient;

  gradient = make_vec3f(0, 0, 0);

  return gradient;
}

// ray.time is set to interval length of intersected sample
inline void TetrahedralVolume_intersect(void *uniform _self, varying Ray &ray)
{
  // Cast to the actual Volume subtype.
  TetrahedralVolume *uniform self = (TetrahedralVolume * uniform) _self;

  // print("TetrahedralVolume_intersect\n");

  // The recommended step size for ray casting based volume renderers.
  const uniform float step =
      self->super.samplingStep / self->super.samplingRate;

  // Compute the next hit point using a spatial acceleration structure.
  // GridAccelerator_intersect(volume->accelerator, step, ray);

  vec3f coord = ray.org + ray.t0 * ray.dir;

  ray.t0 += step;
  ray.time = step;

  ray.geomID = 1;
  ray.primID = 1;
  ray.instID = 1;
}

// ray.time is set to interval length of intersected sample
inline void TetrahedralVolume_intersectAdaptive(
    void *uniform _self, varying Ray &ray, const varying float samplingRate)
{
  // Cast to the actual Volume subtype.
  TetrahedralVolume *uniform self = (TetrahedralVolume * uniform) _self;

  // print("TetrahedralVolume_intersectAdaptive\n\n");
  // print("samplingRate = %", samplingRate);

  // The recommended step size for ray casting based volume renderers.
  const varying float step = self->super.samplingStep / samplingRate;

  // print("samplingStep = %\n\n", volume->super.samplingStep);

  // Compute the next hit point using a spatial acceleration structure.
  // GridAccelerator_intersectAdaptive(volume->accelerator, step, ray);

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

  print("TetrahedralVolume_intersectIsosurface\n");
  // The nominal step size for ray casting based volume renderers, not
  // considering the sampling rate.
  const uniform float step = self->super.samplingStep;

  // Compute the next hit point using a spatial acceleration structure.
  // GridAccelerator_intersectIsosurface(volume->accelerator, step, isovalues,
  // numIsovalues, ray);

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
  self->super.computeSample       = TetrahedralVolume_computeSample;
  self->super.computeGradient     = TetrahedralVolume_computeGradient;
  self->super.intersect           = TetrahedralVolume_intersect;
  self->super.intersectAdaptive   = TetrahedralVolume_intersectAdaptive;
  self->super.intersectIsosurface = TetrahedralVolume_intersectIsosurface;

  self->embree_device = rtcNewDevice(NULL);
  self->embree_scene =
      rtcDeviceNewScene(self->embree_device,
                        RTC_SCENE_STATIC,
                        RTC_INTERSECT_UNIFORM | RTC_INTERSECT_VARYING);
}

export void *uniform
TetrahedralVolume_createInstance(void *uniform cppEquivalent)
{
  TetrahedralVolume *uniform self = uniform new uniform TetrahedralVolume;

  TetrahedralVolume_Constructor(self, cppEquivalent);

  return self;
}

void getTetBBox(const uniform TetrahedralVolume *uniform self,
                uniform size_t id,
                uniform box4f &bbox)
{
  uniform vec4i t =
      self->tetrahedra[id];  // The 4 corner indices of the tetrahedron.

  uniform float &x0 = bbox.lower.x, &y0 = bbox.lower.y, &z0 = bbox.lower.z,
                &x1 = bbox.upper.x, &y1 = bbox.upper.y, &z1 = bbox.upper.z,
                &val0 = bbox.lower.w, &val1 = bbox.upper.w;

  for (uniform int i = 0; i < 4; i++) {
    uniform vec3f p   = self->vertices[getVec4iComponent(t, i)];
    uniform float val = self->field[getVec4iComponent(t, i)];
    if (i == 0) {
      x0 = x1 = p.x;
      y0 = y1 = p.y;
      z0 = z1 = p.z;
      val0 = val1 = val;
    } else {
      if (p.x < x0)
        x0 = p.x;
      if (p.x > x1)
        x1 = p.x;

      if (p.y < y0)
        y0 = p.y;
      if (p.y > y1)
        y1 = p.y;

      if (p.z < z0)
        z0 = p.z;
      if (p.z > z1)
        z1 = p.z;

      if (val < val0)
        val0 = val;
      if (val > val1)
        val1 = val;
    }
  }
}

void tetBounds4Func(void *uniform userData,
                    uniform uint64 id,
                    uniform box4f &bounds)
{
  const uniform TetrahedralVolume *uniform self =
      (const uniform TetrahedralVolume *uniform)userData;

  uniform box4f bbox = self->tetBBoxes[id];

  bounds = bbox;

  // print("ispc tet % bbox: lower(% % %), upper(% % %)\n\n", id, bbox.lower.x,
  // bbox.lower.y, bbox.lower.z, bbox.upper.x, bbox.upper.y, bbox.upper.z);
}

#if (RTCORE_VERSION_MAJOR > 2 || \
     (RTCORE_VERSION_MAJOR == 2 && RTCORE_VERSION_MINOR >= 16))
unmasked void tetBoundsFunc(void *uniform userData,
                            uniform uintptr_t id,
                            uniform RTCBounds &bounds_o)
#else
unmasked void tetBoundsFunc(void *uniform userData,
                            uniform size_t id,
                            uniform RTCBounds &bounds_o)
#endif
{
  const uniform TetrahedralVolume *uniform self =
      (const uniform TetrahedralVolume *uniform)userData;

  uniform box4f bbox = self->tetBBoxes[id];

  bounds_o.lower_x = bbox.lower.x;
  bounds_o.lower_y = bbox.lower.y;
  bounds_o.lower_z = bbox.lower.z;
  bounds_o.upper_x = bbox.upper.x;
  bounds_o.upper_y = bbox.upper.y;
  bounds_o.upper_z = bbox.upper.z;

  // print("ispc tet % bbox: lower(% % %), upper(% % %)\n\n", id, bbox.lower.x,
  // bbox.lower.y, bbox.lower.z, bbox.upper.x, bbox.upper.y, bbox.upper.z);
}

#if (RTCORE_VERSION_MAJOR > 2 || \
     (RTCORE_VERSION_MAJOR == 2 && RTCORE_VERSION_MINOR >= 16))
void tetIntersectFunc(void *uniform userData,
                      varying RTCRay &ray,
                      uniform uintptr_t id)
#else
void tetIntersectFunc(void *uniform userData,
                      varying RTCRay &ray,
                      uniform size_t id)
#endif
{
  const uniform TetrahedralVolume *uniform self =
      (const uniform TetrahedralVolume *uniform)userData;

  uniform box4f bbox = self->tetBBoxes[id];

  vec3f world = make_vec3f(ray.orgx, ray.orgy, ray.orgz);

  if (isLessThan(world, bbox.lower) || isGreaterThan(world, bbox.upper)) {
    return;
  }

  uniform vec4i t =
      self->tetrahedra[id];  // The 4 corner indices of the tetrahedron.

  uniform vec3f p0 = self->vertices[t.x];
  uniform vec3f p1 = self->vertices[t.y];
  uniform vec3f p2 = self->vertices[t.z];
  uniform vec3f p3 = self->vertices[t.w];

  uniform vec3f *uniform faceNormals = self->faceNormals[id].normals;
  uniform vec3f norm0                = faceNormals[0];
  uniform vec3f norm1                = faceNormals[1];
  uniform vec3f norm2                = faceNormals[2];
  uniform vec3f norm3                = faceNormals[3];

  // Distance from the world point to the faces.
  float d0 = dot(norm0, p1 - world);
  float d1 = dot(norm1, p2 - world);
  float d2 = dot(norm2, p3 - world);
  float d3 = dot(norm3, p0 - world);

  if (d0 > 0 && d1 > 0 && d2 > 0 && d3 > 0) {
    ray.primID = id;
    ray.geomID = self->geomID;
  }
}

export void TetrahedralVolume_set(void *uniform _self,
                                  const uniform int &_nVertices,
                                  const uniform int &_nTetrahedra,
                                  const uniform box3f &_bbox,
                                  const vec3f *uniform _vertices,
                                  const vec4i *uniform _tetrahedra,
                                  const float *uniform _field,
                                  uniform int64 rootRef,
                                  const void *uniform _bvhNode,
                                  int64 *uniform _bvhPrimID,
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

  uniform int nNiceVerts = _nVertices / programCount;
  if (_nVertices % programCount != 0) {
    nNiceVerts = nNiceVerts + 1;
  }

  self->nNiceVerts    = nNiceVerts;
  self->nice_vertices = uniform new varying vec3f[nNiceVerts];

  for (uniform int i = 0; i < self->nNiceVerts; i++) {
    varying int index = i * programCount + programIndex;
    if (index < _nVertices) {
      self->nice_vertices[i] = self->vertices[index];
    }
  }

  for (uniform int i = 0; i < self->nVertices; i++) {
    uniform vec3f v = self->vertices[i];
    uniform float x = v.x;
    uniform float y = v.y;
    uniform float z = v.z;
    // print("ispc verts % : % % %\n", i, x, y, z);
  }

  self->faceNormals = uniform new uniform TetFaceNormal[_nTetrahedra];

  self->tetBBoxes = uniform new uniform box4f[_nTetrahedra];

  for (uniform int i = 0; i < self->nTetrahedra; i++) {
    uniform vec4i t =
        self->tetrahedra[i];  // The 4 corner indices of the tetrahedron.

    uniform box4f &bbox = self->tetBBoxes[i];
    getTetBBox(self, i, bbox);

    // The corners of each triangle in the tetrahedron.
    uniform int faces[4][3] = {{1, 2, 3}, {2, 0, 3}, {3, 0, 1}, {0, 2, 1}};

    uniform vec3f *uniform faceNormals = self->faceNormals[i].normals;

    for (uniform int j = 0; j < 4; j++) {
      uniform int t0 = getVec4iComponent(t, faces[j][0]);
      uniform int t1 = getVec4iComponent(t, faces[j][1]);
      uniform int t2 = getVec4iComponent(t, faces[j][2]);

      uniform vec3f p0 = self->vertices[t0];
      uniform vec3f p1 = self->vertices[t1];
      uniform vec3f p2 = self->vertices[t2];

      uniform vec3f q0 = p1 - p0;
      uniform vec3f q1 = p2 - p0;

      uniform vec3f norm = normalize(cross(q0, q1));

      faceNormals[j] = norm;
    }

    // print("ispc tets % : % % % %\n", i, t.x, t.y, t.z, t.w);
  }

  for (uniform int i = 0; i < self->nVertices; i++) {
    uniform float v = self->field[i];
    // print("ispc field % : %\n", i, v);
  }

  // Set up embree callbacks.
  self->geomID = rtcNewUserGeometry(self->embree_scene, self->nTetrahedra);
  rtcSetUserData(self->embree_scene, self->geomID, self);
  rtcSetBoundsFunction(self->embree_scene, self->geomID, tetBoundsFunc);
  rtcSetIntersectFunction(self->embree_scene, self->geomID, tetIntersectFunc);
  rtcSetOccludedFunction(self->embree_scene, self->geomID, tetIntersectFunc);
  rtcCommit(self->embree_scene);

  // self->bvh.intersectAndSamplePrimFunc = intersectAndSampleTet;
  // self->bvh.primBoundsFunc = tetBounds4Func;
  // self->bvh.userData = self;
  // self->bvh.nPrims = self->nTetrahedra;
  self->bvh.rootRef = rootRef;
  self->bvh.node    = (MinMaxBVH2Node * uniform) _bvhNode;
  self->bvh.primID  = _bvhPrimID;
}
