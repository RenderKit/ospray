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
} // namespace ispc
#else
};
#endif // __cplusplus
