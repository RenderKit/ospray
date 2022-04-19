// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GeometryShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Mesh
{
  Geometry super;
  Data1D index;
  Data1D vertex;
  Data1D normal;
  Data1D color;
  Data1D texcoord;
  uint8 **motionVertex;
  uint8 **motionNormal;
  uint32 motionKeys;
  box1f time;
  int64 flagMask; // which attributes are missing and cannot be interpolated
  bool has_alpha; // 4th color component is valid
  bool is_triangleMesh;
  bool isColorFaceVarying;
  bool isTexcoordFaceVarying;
  bool isNormalFaceVarying;

#ifdef __cplusplus
  Mesh()
      : motionVertex(nullptr),
        motionNormal(nullptr),
        motionKeys(0),
        time(0.f, 1.f),
        flagMask(-1),
        has_alpha(false),
        is_triangleMesh(false),
        isColorFaceVarying(false),
        isTexcoordFaceVarying(false),
        isNormalFaceVarying(false)
  {}
};

#ifdef OSPRAY_TARGET_DPCPP
void *QuadMesh_postIntersect_addr();
void *TriangleMesh_postIntersect_addr();
void *Mesh_sampleArea_addr();
void *Mesh_getAreas_addr();

SYCL_EXTERNAL void QuadMesh_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags);

SYCL_EXTERNAL void TriangleMesh_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags);

SYCL_EXTERNAL SampleAreaRes Mesh_sampleArea(const Geometry *uniform const _self,
    const int32 primID,
    const uniform affine3f &xfm,
    const uniform affine3f &,
    const vec2f &s,
    const float time);

SYCL_EXTERNAL void Mesh_getAreas(const Geometry *const uniform _self,
    const int32 *const uniform primIDs,
    const uniform int32 numPrims,
    const uniform affine3f &xfm,
    float *const uniform areas);
#endif

} // namespace ispc
#else
};
#endif // __cplusplus
