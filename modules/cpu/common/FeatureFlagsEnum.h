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

#ifdef __cplusplus
#include <cstdint>
#include <type_traits>
namespace ospray {
#endif // __cplusplus

// NOTE: This enum must be binary compatible with Embree RTCFeatureFlags
enum FeatureFlagsGeometry
#ifdef __cplusplus
    : uint32_t
#endif
{
  FFG_NONE = 0,

  FFG_TRIANGLE = 1 << 1,
  FFG_QUAD = 1 << 2,
  FFG_GRID = 1 << 3,

  FFG_SUBDIVISION = 1 << 4,

  FFG_CONE_LINEAR_CURVE = 1 << 5,
  FFG_ROUND_LINEAR_CURVE = 1 << 6,
  FFG_FLAT_LINEAR_CURVE = 1 << 7,

  FFG_ROUND_BEZIER_CURVE = 1 << 8,
  FFG_FLAT_BEZIER_CURVE = 1 << 9,
  FFG_NORMAL_ORIENTED_BEZIER_CURVE = 1 << 10,

  FFG_ROUND_BSPLINE_CURVE = 1 << 11,
  FFG_FLAT_BSPLINE_CURVE = 1 << 12,
  FFG_NORMAL_ORIENTED_BSPLINE_CURVE = 1 << 13,

  FFG_ROUND_HERMITE_CURVE = 1 << 14,
  FFG_FLAT_HERMITE_CURVE = 1 << 15,
  FFG_NORMAL_ORIENTED_HERMITE_CURVE = 1 << 16,

  FFG_ROUND_CATMULL_ROM_CURVE = 1 << 17,
  FFG_FLAT_CATMULL_ROM_CURVE = 1 << 18,
  FFG_NORMAL_ORIENTED_CATMULL_ROM_CURVE = 1 << 19,

  FFG_SPHERE = 1 << 20,
  FFG_DISC_POINT = 1 << 21,
  FFG_ORIENTED_DISC_POINT = 1 << 22,

  FFG_CURVES = 1 << 5 | 1 << 6 | 1 << 7 | 1 << 8 | 1 << 9 | 1 << 10 | 1 << 11
      | 1 << 12 | 1 << 13 | 1 << 14 | 1 << 15 | 1 << 16 | 1 << 17 | 1 << 18
      | 1 << 19,

  FFG_USER_GEOMETRY =
      1 << 26, // RTC_FEATURE_FLAG_USER_GEOMETRY_CALLBACK_IN_ARGUMENTS

  // OSPRay specific flags
  FFG_BOX = 1 << 29,
  FFG_PLANE = 1 << 30,
  FFG_ISOSURFACE = 1u << 31,

  FFG_OSPRAY_MASK = 1 << 29 | 1 << 30 | 1u << 31,

  FFG_ALL = 0xffffffff
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
