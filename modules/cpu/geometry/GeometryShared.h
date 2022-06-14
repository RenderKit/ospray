// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
typedef void *Geometry_postIntersectFct;
typedef void *Geometry_GetAreasFct;
typedef void *Geometry_SampleAreaFct;
#else
struct Geometry;

// Geometries are supposed to fill certain members of DifferentialGeometry:
// calculate Ng, Ns, st, color, and materialID if the respective bit DG_NG,
// DG_NS, DG_TEXCOORD, DG_COLOR, and DG_MATERIALID, in flags is set.
// Important with instancing: P and ray are in world-coordinates, whereas Ng
// and Ns are in object-coordinates and transformed to world-space by
// Instance_postIntersect.
// World::postIntersect already set the hit point P, color, geometry, and
// material before, and handles normalization/faceforwarding
// (DG_NORMALIZE/DG_FACEFORWARD) after Geometry_postIntersectFct is called.
// Thus the material pointer only needs to be set if different to
// geometry->material, or the color when different to vec4f(1.0f).
typedef void (*Geometry_postIntersectFct)(const Geometry *uniform self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags);

typedef void (*Geometry_GetAreasFct)(const Geometry *const uniform,
    const int32 *const uniform primIDs, // primitive IDs
    const uniform int32 numPrims, // number of primitives
    const uniform affine3f &xfm, // instance transformation (obj2world)
    float *const uniform
        areas // array to return area per primitive in world-space
);

struct SampleAreaRes
{
  vec3f pos; // sampled point, in world-space
  vec3f normal; // geometry normal Ng at the sampled point
};

// sample the given primitive uniformly wrt. area
typedef SampleAreaRes (*Geometry_SampleAreaFct)(const Geometry *const uniform,
    const int32 primID,
    const uniform affine3f &xfm, // instance transformation (obj2world)
    const uniform affine3f &rcp_xfm, // inverse transformation (world2obj)
    const vec2f &s, // random numbers to generate the sample
    const float time // for deformation motion blur
);
#endif // __cplusplus

struct Geometry
{
  // 'virtual' post-intersect function that fills in a
  // DifferentialGeometry struct, see above prototype for details
  Geometry_postIntersectFct postIntersect;

  Geometry_GetAreasFct getAreas;
  Geometry_SampleAreaFct sampleArea;

  // number of primitives this geometry has
  int32 numPrimitives;

#ifdef __cplusplus
  Geometry()
      : postIntersect(nullptr),
        getAreas(nullptr),
        sampleArea(nullptr),
        numPrimitives(0)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
