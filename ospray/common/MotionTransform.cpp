// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "MotionTransform.h"

namespace ospray {

void MotionTransform::readParams(ManagedObject &obj)
{
  transforms = obj.getParamDataT<affine3f>("motion.transform");
  if (!transforms) { // add single transform
    auto data = new Data(OSP_AFFINE3F, vec3ul(1));
    auto &dataA3f = data->as<affine3f>();
    transform = obj.getParam<affine3f>(
        "transform", obj.getParam<affine3f>("xfm", affine3f(one)));
    *dataA3f.data() = transform;
    transforms = &dataA3f;
    data->refDec();
  }
  motionBlur = transforms->size() > 1;
  if (motionBlur)
    time = obj.getParam<range1f>("time", range1f(0.0f, 1.0f));
}

void MotionTransform::setEmbreeTransform(RTCGeometry embreeGeometry) const
{
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
  rtcCommitGeometry(embreeGeometry);
}

} // namespace ospray
