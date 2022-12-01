// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

#if defined(__cplusplus) && !defined(OSPRAY_TARGET_SYCL)
typedef void *Camera_initRay;
#else
struct Camera;
struct CameraSample;
struct Ray;

// Fct pointer type for 'virtual' method that sets a pixel,
// generated ray.dir must be normalized to ensure ray.t is world-space distance
typedef void (*Camera_initRay)(const Camera *uniform,
    varying Ray &ray,
    const varying CameraSample &sample);
#endif

enum CameraType
{
  CAMERA_TYPE_PERSPECTIVE,
  CAMERA_TYPE_ORTHOGRAPHIC,
  CAMERA_TYPE_PANORAMIC,
  CAMERA_TYPE_UNKNOWN,
};

struct Camera
{
  CameraType type;
  Camera_initRay initRay; // the 'virtual' initRay() method

  bool motionBlur; // for the camera itself only, not in general

  float nearClip;
  box2f subImage; // viewable tile / subrange to compute, in [0..1]^2 x [0..1]^2
  range1f shutter; // camera shutter open start and end time, in [0..1]
  bool globalShutter;
  bool rollingShutterHorizontal;
  float rollingShutterDuration;
  RTCGeometry geom; // only to access rtcGetGeometryTransform

#ifdef __cplusplus
  Camera()
      : type(CAMERA_TYPE_UNKNOWN),
        initRay(nullptr),
        motionBlur(false),
        nearClip(1e-6f),
        subImage(0.f),
        shutter(0.f),
        globalShutter(false),
        rollingShutterHorizontal(false),
        rollingShutterDuration(0.f),
        geom(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
