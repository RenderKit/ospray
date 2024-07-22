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
struct RayCone;

// Fct pointer type for 'virtual' method that sets a pixel,
// generated ray.dir must be normalized to ensure ray.t is world-space distance
// generated rayCone is in normalized camera coordinates for y (height)
typedef void (*Camera_initRay)(const Camera *uniform,
    varying Ray &ray,
    varying RayCone &rayCone,
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

  float nearClip;
  box2f subImage; // viewable tile / subrange to compute, in [0..1]^2 x [0..1]^2
  range1f shutter; // camera shutter open start and end time, in [0..1]
  bool globalShutter;
  bool motionBlur; // for the camera itself only, not in general
  bool needLensSample;
  bool needTimeSample;
  bool rollingShutterHorizontal;
  float rollingShutterDuration;
  RTCScene scene; // only to call rtcGetGeometryTransformFromScene

#ifdef __cplusplus
  Camera()
      : type(CAMERA_TYPE_UNKNOWN),
        initRay(nullptr),
        nearClip(1e-6f),
        subImage(0.f),
        shutter(0.f),
        globalShutter(false),
        motionBlur(false),
        needLensSample(false),
        needTimeSample(false),
        rollingShutterHorizontal(false),
        rollingShutterDuration(0.f),
        scene(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
