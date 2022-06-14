// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Camera.h"
#include "common/Util.h"

namespace ospray {

static FactoryMap<Camera> g_cameraMap;

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

box3f Camera::projectBox(const box3f &) const
{
  return box3f(vec3f(0.f), vec3f(1.f));
}

void Camera::commit()
{
  motionTransform.readParams(*this);

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
  shutterType =
      (OSPShutterType)getParam<uint8_t>("shutterType", OSP_SHUTTER_GLOBAL);
  rollingShutterDuration = clamp(
      getParam<float>("rollingShutterDuration", 0.0f), 0.0f, shutter.size());

  if (motionTransform.motionBlur || motionTransform.quaternion) {
    // create dummy RTCGeometry for transform interpolation or conversion
    if (!embreeGeometry)
      embreeGeometry = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);

    motionTransform.setEmbreeTransform(embreeGeometry);

    if (shutter.lower == shutter.upper || !motionTransform.motionBlur) {
      // directly interpolate to single shutter time
      rtcGetGeometryTransform(embreeGeometry,
          shutter.lower,
          RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR,
          &motionTransform.transform);
      motionTransform.motionBlur = false;
    }
  }

  if (!motionTransform.motionBlur) {
    if (embreeGeometry) {
      rtcReleaseGeometry(embreeGeometry);
      embreeGeometry = nullptr;
    }
    // apply transform right away
    pos = xfmPoint(motionTransform.transform, pos);
    dir = normalize(xfmVector(motionTransform.transform, dir));
    up = normalize(xfmVector(motionTransform.transform, up));
  }

  if (shutterType != OSP_SHUTTER_GLOBAL) { // rolling shutter
    shutter.upper -= rollingShutterDuration;
    if (shutterType == OSP_SHUTTER_ROLLING_LEFT
        || shutterType == OSP_SHUTTER_ROLLING_DOWN)
      std::swap(shutter.lower, shutter.upper);
  }

  getSh()->nearClip = nearClip;
  getSh()->subImage.lower = imageStart;
  getSh()->subImage.upper = imageEnd;
  getSh()->shutter = shutter;
  getSh()->motionBlur = motionTransform.motionBlur;
  getSh()->geom = embreeGeometry;
  getSh()->globalShutter = shutterType == OSP_SHUTTER_GLOBAL;
  getSh()->rollingShutterHorizontal = (shutterType == OSP_SHUTTER_ROLLING_RIGHT
      || shutterType == OSP_SHUTTER_ROLLING_LEFT);
  getSh()->rollingShutterDuration = rollingShutterDuration;
}

void Camera::setDevice(RTCDevice device)
{
  embreeDevice = device;
}

OSPTYPEFOR_DEFINITION(Camera *);

} // namespace ospray
