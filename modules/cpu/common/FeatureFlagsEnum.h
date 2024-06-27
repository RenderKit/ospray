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
#include <iostream> // for debug prints
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
  FFG_LINEAR_CURVE = RTC_FEATURE_FLAG_LINEAR_CURVES,

  FFG_ROUND_BEZIER_CURVE = RTC_FEATURE_FLAG_ROUND_BEZIER_CURVE,
  FFG_FLAT_BEZIER_CURVE = RTC_FEATURE_FLAG_FLAT_BEZIER_CURVE,
  FFG_NORMAL_ORIENTED_BEZIER_CURVE =
      RTC_FEATURE_FLAG_NORMAL_ORIENTED_BEZIER_CURVE,
  FFG_BEZIER_CURVE = RTC_FEATURE_FLAG_BEZIER_CURVES,

  FFG_ROUND_BSPLINE_CURVE = RTC_FEATURE_FLAG_ROUND_BSPLINE_CURVE,
  FFG_FLAT_BSPLINE_CURVE = RTC_FEATURE_FLAG_FLAT_BSPLINE_CURVE,
  FFG_NORMAL_ORIENTED_BSPLINE_CURVE =
      RTC_FEATURE_FLAG_NORMAL_ORIENTED_BSPLINE_CURVE,
  FFG_BSPLINE_CURVE = RTC_FEATURE_FLAG_BSPLINE_CURVES,

  FFG_ROUND_HERMITE_CURVE = RTC_FEATURE_FLAG_ROUND_HERMITE_CURVE,
  FFG_FLAT_HERMITE_CURVE = RTC_FEATURE_FLAG_FLAT_HERMITE_CURVE,
  FFG_NORMAL_ORIENTED_HERMITE_CURVE =
      RTC_FEATURE_FLAG_NORMAL_ORIENTED_HERMITE_CURVE,
  FFG_HERMITE_CURVE = RTC_FEATURE_FLAG_HERMITE_CURVES,

  FFG_ROUND_CATMULL_ROM_CURVE = RTC_FEATURE_FLAG_ROUND_CATMULL_ROM_CURVE,
  FFG_FLAT_CATMULL_ROM_CURVE = RTC_FEATURE_FLAG_FLAT_CATMULL_ROM_CURVE,
  FFG_NORMAL_ORIENTED_CATMULL_ROM_CURVE =
      RTC_FEATURE_FLAG_NORMAL_ORIENTED_CATMULL_ROM_CURVE,

  FFG_CATMULL_ROM_CURVE = 0 | RTC_FEATURE_FLAG_ROUND_CATMULL_ROM_CURVE
      | RTC_FEATURE_FLAG_FLAT_CATMULL_ROM_CURVE
      | RTC_FEATURE_FLAG_NORMAL_ORIENTED_CATMULL_ROM_CURVE,

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

  FFO_RENDERER_SHADOWCATCHER = 1 << 30,

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

inline void printFfg(uint32_t flags)
{
  if (flags == FFG_NONE) {
    std::cout << "FFG_NONE" << std::endl;
    return;
  }

  if (flags & FFG_MOTION_BLUR)
    std::cout << "FFG_MOTION_BLUR" << std::endl;
  if (flags & FFG_TRIANGLE)
    std::cout << "FFG_TRIANGLE" << std::endl;
  if (flags & FFG_QUAD)
    std::cout << "FFG_QUAD" << std::endl;
  if (flags & FFG_GRID)
    std::cout << "FFG_GRID" << std::endl;
  if (flags & FFG_SUBDIVISION)
    std::cout << "FFG_SUBDIVISION" << std::endl;

  if (flags & FFG_CONE_LINEAR_CURVE)
    std::cout << "FFG_CONE_LINEAR_CURVE" << std::endl;
  if (flags & FFG_ROUND_LINEAR_CURVE)
    std::cout << "FFG_ROUND_LINEAR_CURVE" << std::endl;
  if (flags & FFG_FLAT_LINEAR_CURVE)
    std::cout << "FFG_FLAT_LINEAR_CURVE" << std::endl;
  if (flags & FFG_LINEAR_CURVE)
    std::cout << "FFG_LINEAR_CURVE" << std::endl;

  if (flags & FFG_ROUND_BEZIER_CURVE)
    std::cout << "FFG_ROUND_BEZIER_CURVE" << std::endl;
  if (flags & FFG_FLAT_BEZIER_CURVE)
    std::cout << "FFG_FLAT_BEZIER_CURVE" << std::endl;
  if (flags & FFG_NORMAL_ORIENTED_BEZIER_CURVE)
    std::cout << "FFG_NORMAL_ORIENTED_BEZIER_CURVE" << std::endl;
  if (flags & FFG_BEZIER_CURVE)
    std::cout << "FFG_BEZIER_CURVE" << std::endl;

  if (flags & FFG_ROUND_BSPLINE_CURVE)
    std::cout << "FFG_ROUND_BSPLINE_CURVE" << std::endl;
  if (flags & FFG_FLAT_BSPLINE_CURVE)
    std::cout << "FFG_FLAT_BSPLINE_CURVE" << std::endl;
  if (flags & FFG_NORMAL_ORIENTED_BSPLINE_CURVE)
    std::cout << "FFG_NORMAL_ORIENTED_BSPLINE_CURVE" << std::endl;
  if (flags & FFG_BSPLINE_CURVE)
    std::cout << "FFG_BSPLINE_CURVE" << std::endl;

  if (flags & FFG_ROUND_HERMITE_CURVE)
    std::cout << "FFG_ROUND_HERMITE_CURVE" << std::endl;
  if (flags & FFG_FLAT_HERMITE_CURVE)
    std::cout << "FFG_FLAT_HERMITE_CURVE" << std::endl;
  if (flags & FFG_NORMAL_ORIENTED_HERMITE_CURVE)
    std::cout << "FFG_NORMAL_ORIENTED_HERMITE_CURVE" << std::endl;
  if (flags & FFG_HERMITE_CURVE)
    std::cout << "FFG_HERMITE_CURVE" << std::endl;

  if (flags & FFG_ROUND_CATMULL_ROM_CURVE)
    std::cout << "FFG_ROUND_CATMULL_ROM_CURVE" << std::endl;
  if (flags & FFG_FLAT_CATMULL_ROM_CURVE)
    std::cout << "FFG_FLAT_CATMULL_ROM_CURVE" << std::endl;
  if (flags & FFG_NORMAL_ORIENTED_CATMULL_ROM_CURVE)
    std::cout << "FFG_NORMAL_ORIENTED_CATMULL_ROM_CURVE" << std::endl;
  if (flags & FFG_CATMULL_ROM_CURVE)
    std::cout << "FFG_CATMULL_ROM_CURVE" << std::endl;

  if (flags & FFG_SPHERE)
    std::cout << "FFG_SPHERE" << std::endl;
  if (flags & FFG_DISC_POINT)
    std::cout << "FFG_DISC_POINT" << std::endl;
  if (flags & FFG_ORIENTED_DISC_POINT)
    std::cout << "FFG_ORIENTED_DISC_POINT" << std::endl;

  if (flags & FFG_SPHERES)
    std::cout << "FFG_SPHERES" << std::endl;

  if (flags & FFG_CURVES)
    std::cout << "FFG_CURVES" << std::endl;

  // OSPRay specific flags
  if (flags & FFG_BOX)
    std::cout << "FFG_BOX" << std::endl;
  if (flags & FFG_PLANE)
    std::cout << "FFG_PLANE" << std::endl;
  if (flags & FFG_ISOSURFACE)
    std::cout << "FFG_ISOSURFACE" << std::endl;
}

inline void printFfo(uint32_t flags)
{
  if (flags == FFO_NONE) {
    std::cout << "FFO_NONE" << std::endl;
    return;
  }

  if (flags & FFO_FB_LOCAL)
    std::cout << "FFO_FB_LOCAL" << std::endl;
  if (flags & FFO_FB_SPARSE)
    std::cout << "FFO_FB_SPARSE" << std::endl;

  if (flags & FFO_CAMERA_PERSPECTIVE)
    std::cout << "FFO_CAMERA_PERSPECTIVE" << std::endl;
  if (flags & FFO_CAMERA_ORTHOGRAPHIC)
    std::cout << "FFO_CAMERA_ORTHOGRAPHIC" << std::endl;
  if (flags & FFO_CAMERA_PANORAMIC)
    std::cout << "FFO_CAMERA_PANORAMIC" << std::endl;

  if (flags & FFO_LIGHT_AMBIENT)
    std::cout << "FFO_LIGHT_AMBIENT" << std::endl;
  if (flags & FFO_LIGHT_CYLINDER)
    std::cout << "FFO_LIGHT_CYLINDER" << std::endl;
  if (flags & FFO_LIGHT_DIRECTIONAL)
    std::cout << "FFO_LIGHT_DIRECTIONAL" << std::endl;
  if (flags & FFO_LIGHT_HDRI)
    std::cout << "FFO_LIGHT_HDRI" << std::endl;
  if (flags & FFO_LIGHT_POINT)
    std::cout << "FFO_LIGHT_POINT" << std::endl;
  if (flags & FFO_LIGHT_QUAD)
    std::cout << "FFO_LIGHT_QUAD" << std::endl;
  if (flags & FFO_LIGHT_SPOT)
    std::cout << "FFO_LIGHT_SPOT" << std::endl;
  if (flags & FFO_LIGHT_GEOMETRY)
    std::cout << "FFO_LIGHT_GEOMETRY" << std::endl;

  if (flags & FFO_MATERIAL_ALLOY)
    std::cout << "FFO_MATERIAL_ALLOY" << std::endl;
  if (flags & FFO_MATERIAL_CARPAINT)
    std::cout << "FFO_MATERIAL_CARPAINT" << std::endl;
  if (flags & FFO_MATERIAL_GLASS)
    std::cout << "FFO_MATERIAL_GLASS" << std::endl;
  if (flags & FFO_MATERIAL_LUMINOUS)
    std::cout << "FFO_MATERIAL_LUMINOUS" << std::endl;
  if (flags & FFO_MATERIAL_METAL)
    std::cout << "FFO_MATERIAL_METAL" << std::endl;
  if (flags & FFO_MATERIAL_METALLICPAINT)
    std::cout << "FFO_MATERIAL_METALLICPAINT" << std::endl;
  if (flags & FFO_MATERIAL_MIX)
    std::cout << "FFO_MATERIAL_MIX" << std::endl;
  if (flags & FFO_MATERIAL_OBJ)
    std::cout << "FFO_MATERIAL_OBJ" << std::endl;
  if (flags & FFO_MATERIAL_PLASTIC)
    std::cout << "FFO_MATERIAL_PLASTIC" << std::endl;
  if (flags & FFO_MATERIAL_PRINCIPLED)
    std::cout << "FFO_MATERIAL_PRINCIPLED" << std::endl;
  if (flags & FFO_MATERIAL_THINGLASS)
    std::cout << "FFO_MATERIAL_THINGLASS" << std::endl;
  if (flags & FFO_MATERIAL_VELVET)
    std::cout << "FFO_MATERIAL_VELVET" << std::endl;

  if (flags & FFO_TEXTURE_IN_MATERIAL)
    std::cout << "FFO_TEXTURE_IN_MATERIAL" << std::endl;
  if (flags & FFO_TEXTURE_IN_RENDERER)
    std::cout << "FFO_TEXTURE_IN_RENDERER" << std::endl;

  if (flags & FFO_VOLUME_IN_SCENE)
    std::cout << "FFO_VOLUME_IN_SCENE" << std::endl;
  if (flags & FFO_VOLUME_SCIVIS_SHADING)
    std::cout << "FFO_VOLUME_SCIVIS_SHADING" << std::endl;

  if (flags & FFO_CAMERA_MOTION_BLUR)
    std::cout << "FFO_CAMERA_MOTION_BLUR" << std::endl;

  if (flags & FFO_RENDERER_SHADOWCATCHER)
    std::cout << "FFO_RENDERER_SHADOWCATCHER" << std::endl;
}
#endif

#ifdef __cplusplus
}
// namespace ospray
#endif
