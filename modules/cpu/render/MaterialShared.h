// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#define EPS 1e-5f

#include "bsdfs/MicrofacetAlbedoTablesShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

#if defined(__cplusplus) && !defined(OSPRAY_TARGET_SYCL)
typedef void *Material_GetBSDFFunc;
typedef void *Material_GetTransparencyFunc;
typedef void *Material_SelectNextMediumFunc;
#else
struct BSDF;
struct ShadingContext;
struct Medium;
struct DifferentialGeometry;
struct Ray;
struct Material;
struct FeatureFlagsHandler;

typedef const varying BSDF *uniform (*Material_GetBSDFFunc)(
    const uniform Material *uniform self,
    uniform ShadingContext *uniform ctx,
    // The point to shade on a surface.
    const DifferentialGeometry &dg,
    // The ray arriving at the point to shade.
    const Ray &ray,
    // The medium this ray travels inside.
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &ffh);

// shortcut: compute transmission of material, for transparent shadows,
// neglecting refraction
typedef vec3f (*Material_GetTransparencyFunc)(
    const uniform Material *uniform self,
    // The point to shade on a surface.
    const DifferentialGeometry &dg,
    // The ray arriving at the point to shade.
    const Ray &ray,
    // The medium this ray travels inside.
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &ffh);

typedef void (*Material_SelectNextMediumFunc)(
    const uniform Material *uniform self,
    const DifferentialGeometry &dg,
    Medium &currentMedium);
#endif

enum MaterialType
{
  MATERIAL_TYPE_ALLOY = 0,
  MATERIAL_TYPE_CARPAINT = 1,
  MATERIAL_TYPE_GLASS = 2,
  MATERIAL_TYPE_LUMINOUS = 3,
  MATERIAL_TYPE_METAL = 4,
  MATERIAL_TYPE_METALLICPAINT = 5,
  MATERIAL_TYPE_MIX = 6,
  MATERIAL_TYPE_OBJ = 7,
  MATERIAL_TYPE_PLASTIC = 8,
  MATERIAL_TYPE_PRINCIPLED = 9,
  MATERIAL_TYPE_THINGLASS = 10,
  MATERIAL_TYPE_VELVET = 11,
  MATERIAL_TYPE_UNKNOWN = 12
};

// ISPC-side abstraction for a material.
struct Material
{
  MaterialType type;
  Material_GetBSDFFunc getBSDF;
  Material_GetTransparencyFunc getTransparency;
  Material_SelectNextMediumFunc selectNextMedium;
  vec3f emission; // simple constant (spatially and angular) emission, returns
                  // radiance; TODO SV-EDFs

  MicrofacetAlbedoTables *microfacetAlbedoTables;

#ifdef __cplusplus
  Material(const vec3f &emission = vec3f(0.f))
      : type(MATERIAL_TYPE_UNKNOWN),
        getBSDF(nullptr),
        getTransparency(nullptr),
        selectNextMedium(nullptr),
        emission(emission),
        microfacetAlbedoTables(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
