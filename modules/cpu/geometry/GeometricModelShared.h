// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GeometryShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct GeometricModel
{
  Geometry *geom;

  Data1D color;
  Data1D index; // per-primitive property mapping
  Data1D material;

  bool useRendererMaterialList;
  bool invertedNormals;

  float areaPDF;

#ifdef __cplusplus
  GeometricModel()
      : geom(nullptr),
        useRendererMaterialList(false),
        invertedNormals(false),
        areaPDF(0.f)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
