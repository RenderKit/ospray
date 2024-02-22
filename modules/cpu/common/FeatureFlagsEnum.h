// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef OSPRAY_ENABLE_VOLUMES
#ifdef ISPC
#include <openvkl/openvkl.isph>
#else
#include <openvkl/openvkl.h>
#endif
#endif

#include "common/Embree.h"

#ifdef __cplusplus
#include <cstdint>
#include <type_traits>
namespace ospray {
#endif // __cplusplus

enum FeatureFlagsGeometry
#ifdef __cplusplus
    : uint32_t
#endif
{
  FFG_NONE = RTC_FEATURE_FLAG_NONE,

  FFG_MOTION_BLUR = RTC_FEATURE_FLAG_MOTION_BLUR,

  FFG_TRIANGLE = RTC_FEATURE_FLAG_TRIANGLE,
  FFG_QUAD = RTC_FEATURE_FLAG_QUAD,
  FFG_GRID = RTC_FEATURE_FLAG_GRID,
  FFG_SUBDIVISION = RTC_FEATURE_FLAG_SUBDIVISION,

  FFG_CONE_LINEAR_CURVE = RTC_FEATURE_FLAG_CONE_LINEAR_CURVE,
  FFG_ROUND_LINEAR_CURVE = RTC_FEATURE_FLAG_ROUND_LINEAR_CURVE,
  FFG_FLAT_LINEAR_CURVE = RTC_FEATURE_FLAG_FLAT_LINEAR_CURVE,

  FFG_ROUND_BEZIER_CURVE = RTC_FEATURE_FLAG_ROUND_BEZIER_CURVE,
  FFG_FLAT_BEZIER_CURVE = RTC_FEATURE_FLAG_FLAT_BEZIER_CURVE,
  FFG_NORMAL_ORIENTED_BEZIER_CURVE =
      RTC_FEATURE_FLAG_NORMAL_ORIENTED_BEZIER_CURVE,

  FFG_ROUND_BSPLINE_CURVE = RTC_FEATURE_FLAG_ROUND_BSPLINE_CURVE,
  FFG_FLAT_BSPLINE_CURVE = RTC_FEATURE_FLAG_FLAT_BSPLINE_CURVE,
  FFG_NORMAL_ORIENTED_BSPLINE_CURVE =
      RTC_FEATURE_FLAG_NORMAL_ORIENTED_BSPLINE_CURVE,

  FFG_ROUND_HERMITE_CURVE = RTC_FEATURE_FLAG_ROUND_HERMITE_CURVE,
  FFG_FLAT_HERMITE_CURVE = RTC_FEATURE_FLAG_FLAT_HERMITE_CURVE,
  FFG_NORMAL_ORIENTED_HERMITE_CURVE =
      RTC_FEATURE_FLAG_NORMAL_ORIENTED_HERMITE_CURVE,

  FFG_ROUND_CATMULL_ROM_CURVE = RTC_FEATURE_FLAG_ROUND_CATMULL_ROM_CURVE,
  FFG_FLAT_CATMULL_ROM_CURVE = RTC_FEATURE_FLAG_FLAT_CATMULL_ROM_CURVE,
  FFG_NORMAL_ORIENTED_CATMULL_ROM_CURVE =
      RTC_FEATURE_FLAG_NORMAL_ORIENTED_CATMULL_ROM_CURVE,

  FFG_SPHERE = RTC_FEATURE_FLAG_SPHERE_POINT,
  FFG_DISC_POINT = RTC_FEATURE_FLAG_DISC_POINT,
  FFG_ORIENTED_DISC_POINT = RTC_FEATURE_FLAG_ORIENTED_DISC_POINT,

  FFG_SPHERES = RTC_FEATURE_FLAG_POINT,

  FFG_CURVES = RTC_FEATURE_FLAG_CURVES,

  // OSPRay specific flags

  // repurposing unused flags below from Embree for FFG_BOX/PLANE/ISOSURFACE
  FFG_OSPRAY_MASK = 0 | RTC_FEATURE_FLAG_USER_GEOMETRY_CALLBACK_IN_GEOMETRY
      | RTC_FEATURE_FLAG_FILTER_FUNCTION_IN_GEOMETRY
      | RTC_FEATURE_FLAG_32_BIT_RAY_MASK,

  FFG_BOX = 0 | RTC_FEATURE_FLAG_USER_GEOMETRY_CALLBACK_IN_ARGUMENTS
      | RTC_FEATURE_FLAG_USER_GEOMETRY_CALLBACK_IN_GEOMETRY,
  FFG_PLANE = 0 | RTC_FEATURE_FLAG_USER_GEOMETRY_CALLBACK_IN_ARGUMENTS
      | RTC_FEATURE_FLAG_FILTER_FUNCTION_IN_GEOMETRY,
  FFG_ISOSURFACE = 0 | RTC_FEATURE_FLAG_USER_GEOMETRY_CALLBACK_IN_ARGUMENTS
      | RTC_FEATURE_FLAG_32_BIT_RAY_MASK,

  FFG_ALL = RTC_FEATURE_FLAG_ALL
};

enum FeatureFlagsOther
#ifdef __cplusplus
    : uint32_t
#endif
{
  FFO_NONE = 0,

  FFO_FB_LOCAL = 1 << 0,
  FFO_FB_SPARSE = 1 << 1,

  FFO_CAMERA_PERSPECTIVE = 1 << 2,
  FFO_CAMERA_ORTHOGRAPHIC = 1 << 3,
  FFO_CAMERA_PANORAMIC = 1 << 4,

  FFO_LIGHT_AMBIENT = 1 << 5,
  FFO_LIGHT_CYLINDER = 1 << 6,
  FFO_LIGHT_DIRECTIONAL = 1 << 7,
  FFO_LIGHT_HDRI = 1 << 8,
  FFO_LIGHT_POINT = 1 << 9,
  FFO_LIGHT_QUAD = 1 << 10,
  FFO_LIGHT_SPOT = 1 << 11,
  FFO_LIGHT_GEOMETRY = 1 << 12,

  FFO_MATERIAL_ALLOY = 1 << 13,
  FFO_MATERIAL_CARPAINT = 1 << 14,
  FFO_MATERIAL_GLASS = 1 << 15,
  FFO_MATERIAL_LUMINOUS = 1 << 16,
  FFO_MATERIAL_METAL = 1 << 17,
  FFO_MATERIAL_METALLICPAINT = 1 << 18,
  FFO_MATERIAL_MIX = 1 << 19,
  FFO_MATERIAL_OBJ = 1 << 20,
  FFO_MATERIAL_PLASTIC = 1 << 21,
  FFO_MATERIAL_PRINCIPLED = 1 << 22,
  FFO_MATERIAL_THINGLASS = 1 << 23,
  FFO_MATERIAL_VELVET = 1 << 24,

  FFO_TEXTURE_IN_MATERIAL = 1 << 25,
  FFO_TEXTURE_IN_RENDERER = 1 << 26,

  // We track if there's a volume object in the scene separately from the volume
  // feature flags to distinguish between needing the volume rendering/sampling
  // code paths or just needing the isosurface traversal code path.
  FFO_VOLUME_IN_SCENE = 1 << 27,

  // Volume shading in SciVis renderer
  FFO_VOLUME_SCIVIS_SHADING = 1 << 28,

  FFO_CAMERA_MOTION_BLUR = 1 << 29,

  FFO_ALL = 0xffffffff
};

struct FeatureFlags
{
  FeatureFlagsGeometry geometry;

  FeatureFlagsOther other;

#ifdef OSPRAY_ENABLE_VOLUMES
  VKLFeatureFlags volume;
#endif

#ifdef __cplusplus
  constexpr FeatureFlags()
      : geometry(FFG_NONE),
        other(FFO_NONE)
#ifdef OSPRAY_ENABLE_VOLUMES
        ,
        volume(VKL_FEATURE_FLAGS_NONE)
#endif
  {}
  void reset()
  {
    geometry = FFG_NONE;
    other = FFO_NONE;
#ifdef OSPRAY_ENABLE_VOLUMES
    volume = VKL_FEATURE_FLAGS_NONE;
#endif
  }
#endif
};

#ifdef __cplusplus
template <typename T,
    typename = typename std::enable_if<std::is_enum<T>::value>::type>
inline T operator|(T a, T b)
{
  return static_cast<T>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

template <typename T,
    typename = typename std::enable_if<std::is_enum<T>::value>::type>
inline T &operator|=(T &a, T b)
{
  return a = a | b;
}

inline FeatureFlags operator|(const FeatureFlags &a, const FeatureFlags &b)
{
  FeatureFlags ff;
  ff.geometry = a.geometry | b.geometry;
#ifdef OSPRAY_ENABLE_VOLUMES
  ff.volume = a.volume | b.volume;
#endif
  ff.other = a.other | b.other;
  return ff;
}

inline FeatureFlags &operator|=(FeatureFlags &a, const FeatureFlags &b)
{
  a.geometry |= b.geometry;
#ifdef OSPRAY_ENABLE_VOLUMES
  a.volume |= b.volume;
#endif
  a.other |= b.other;
  return a;
}
#endif

#ifdef __cplusplus
}
// namespace ospray
#endif
