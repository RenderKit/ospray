// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Camera.h"
#include "camera/Camera_ispc.h"
#include "common/Util.h"

namespace ospray {

static FactoryMap<Camera> g_cameraMap;

ProjectedPoint::ProjectedPoint(const vec3f &pos, float radius)
    : screenPos(pos), radius(radius)
{}

Camera::Camera()
{
  managedObjectType = OSP_CAMERA;
}

Camera::~Camera()
{
  if (embreeGeometry)
    rtcReleaseGeometry(embreeGeometry);
}

Camera *Camera::createInstance(const char *type)
{
  return createInstanceHelper(type, g_cameraMap[type]);
}

void Camera::registerType(const char *type, FactoryFcn<Camera> f)
{
  g_cameraMap[type] = f;
}

std::string Camera::toString() const
{
  return "ospray::Camera";
}

void Camera::commit()
{
  MotionTransform::commit();

  // "parse" the general expected parameters
  pos = getParam<vec3f>("position", vec3f(0.f));
  dir = getParam<vec3f>("direction", vec3f(0.f, 0.f, 1.f));
  up = getParam<vec3f>("up", vec3f(0.f, 1.f, 0.f));
  nearClip = std::max(getParam<float>("nearClip", 1e-6f), 0.f);
  imageStart = getParam<vec2f>("imageStart", vec2f(0.f));
  imageEnd = getParam<vec2f>("imageEnd", vec2f(1.f));
  shutter = getParam<range1f>("shutter", range1f(0.5f, 0.5f));
  clamp(shutter.lower);
  clamp(shutter.upper);
  if (shutter.lower > shutter.upper)
    shutter.lower = shutter.upper;

  affine3f single_xfm = (*motionTransforms)[0];
  if (motionBlur) { // create dummy RTCGeometry for transform interpolation
    if (!embreeGeometry)
      embreeGeometry = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);

    auto &xfm = *motionTransforms;
    rtcSetGeometryTimeStepCount(embreeGeometry, xfm.size());
    for (size_t i = 0; i < xfm.size(); i++)
      rtcSetGeometryTransform(
          embreeGeometry, i, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &xfm[i]);
    rtcSetGeometryTimeRange(embreeGeometry, time.lower, time.upper);
    rtcCommitGeometry(embreeGeometry);

    if (shutter.lower == shutter.upper) {
      // directly interpolate to single shutter time
      rtcGetGeometryTransform(embreeGeometry,
          shutter.lower,
          RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR,
          &single_xfm);
      motionBlur = false;
    }
  } else if (embreeGeometry) {
    rtcReleaseGeometry(embreeGeometry);
    embreeGeometry = nullptr;
  }

  if (!motionBlur) { // apply transform right away
    pos = xfmPoint(single_xfm, pos);
    dir = xfmVector(single_xfm, dir);
    up = xfmVector(single_xfm, up);
  }

  ispc::Camera_set(getIE(),
      nearClip,
      (const ispc::vec2f &)imageStart,
      (const ispc::vec2f &)imageEnd,
      (const ispc::box1f &)shutter,
      motionBlur,
      embreeGeometry);
}

ProjectedPoint Camera::projectPoint(const vec3f &) const
{
  NOT_IMPLEMENTED;
}

void Camera::setDevice(RTCDevice device)
{
  embreeDevice = device;
}

OSPTYPEFOR_DEFINITION(Camera *);

} // namespace ospray
