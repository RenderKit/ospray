// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ospray {
#endif // __cplusplus

enum FeatureFlagsGeometry
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

  FFG_BOX = 1 << 29,
  FFG_PLANE = 1 << 30,
  FFG_ISOSURFACE = 1 << 31,

  FFG_OSPRAY_MASK = 1 << 29 | 1 << 30 | 1 << 31,

  FFG_ALL = 0xffffffff
};

enum FeatureFlagsVolume
{
  FFV_NONE = 0,

  FFV_VOLUME = 1 << 0,

  FFV_ALL = 0xffffffff
};

enum FeatureFlagsOther
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

  FFO_ALL = 0xffffffff
};

struct FeatureFlags
{
  FeatureFlagsGeometry geometry;
  FeatureFlagsVolume volume;
  FeatureFlagsOther other;
#ifdef __cplusplus
  void setNone()
  {
    geometry = FFG_NONE;
    volume = FFV_NONE;
    other = FFO_NONE;
  }
};

template <typename T>
inline T operator|(T a, T b)
{
  return (T)((unsigned int)(a) | (unsigned int)(b));
}

template <typename T>
inline T &operator|=(T &a, T b)
{
  return (T &)((unsigned int &)(a) |= (unsigned int)(b));
}
} // namespace ospray
#else
};
#endif // __cplusplus
