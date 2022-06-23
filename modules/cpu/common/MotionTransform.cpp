// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "MotionTransform.h"

namespace ospray {

void MotionTransform::readParams(ManagedObject &obj)
{
  transforms = obj.getParamDataT<affine3f>("motion.transform");
  if (transforms) {
    motionBlur = transforms->size() > 1;
    quaternion = false;
    transform = (*transforms)[0];
  } else { // look for SRT
    auto shift = obj.getParamDataT<vec3f>("motion.pivot");
    auto scale = obj.getParamDataT<vec3f>("motion.scale");
    auto rotation = obj.getParamDataT<quatf>("motion.rotation");
    auto translation = obj.getParamDataT<vec3f>("motion.translation");
    quaternion = shift || scale || rotation || translation;
    if (quaternion) {
      // determine (minimum) size
      size_t size = std::numeric_limits<size_t>::max();
      if (shift)
        size = shift->size();
      if (scale)
        size = std::min(size, scale->size());
      if (rotation)
        size = std::min(size, rotation->size());
      if (translation)
        size = std::min(size, translation->size());
      motionBlur = size > 1;
      quaternions.resize(size);
      for (size_t i = 0; i < size; i++) {
        auto *qdecomp = &quaternions[i];
        rtcInitQuaternionDecomposition(qdecomp);
        if (shift) {
          const vec3f &s = (*shift)[i];
          rtcQuaternionDecompositionSetShift(qdecomp, s.x, s.y, s.z);
        }
        if (scale) {
          const vec3f &s = (*scale)[i];
          rtcQuaternionDecompositionSetScale(qdecomp, s.x, s.y, s.z);
        }
        if (rotation) {
          const quatf &q = (*rotation)[i];
          rtcQuaternionDecompositionSetQuaternion(qdecomp, q.r, q.i, q.j, q.k);
        }
        if (translation) {
          const vec3f &t = (*translation)[i];
          rtcQuaternionDecompositionSetTranslation(qdecomp, t.x, t.y, t.z);
        }
      }
    } else { // add single transform
      quaternions.clear();
      motionBlur = false;
      transform = obj.getParam<affine3f>(
          "transform", obj.getParam<affine3f>("xfm", affine3f(one)));
    }
  }
  time = range1f(0.0f, 1.0f);
  if (motionBlur) {
    time = obj.getParam<range1f>("time", range1f(0.0f, 1.0f));
    if (time.upper < time.lower)
      time.upper = time.lower;
  }
}

void MotionTransform::setEmbreeTransform(RTCGeometry embreeGeometry) const
{
  if (quaternion) {
    rtcSetGeometryTimeStepCount(embreeGeometry, quaternions.size());
    for (size_t i = 0; i < quaternions.size(); i++)
      rtcSetGeometryTransformQuaternion(embreeGeometry, i, &quaternions[i]);
    rtcSetGeometryTimeRange(embreeGeometry, time.lower, time.upper);
  } else {
    if (motionBlur) {
      rtcSetGeometryTimeStepCount(embreeGeometry, transforms->size());
      for (size_t i = 0; i < transforms->size(); i++)
        rtcSetGeometryTransform(embreeGeometry,
            i,
            RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR,
            &(*transforms)[i]);
      rtcSetGeometryTimeRange(embreeGeometry, time.lower, time.upper);
    } else {
      rtcSetGeometryTimeStepCount(embreeGeometry, 1);
      rtcSetGeometryTransform(
          embreeGeometry, 0, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, transform);
    }
  }
  rtcCommitGeometry(embreeGeometry);
}

} // namespace ospray
