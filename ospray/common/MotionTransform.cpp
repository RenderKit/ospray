// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "MotionTransform.h"

namespace ospray {

void MotionTransform::commit()
{
  motionTransforms = getParamDataT<affine3f>("motion.transform");
  if (!motionTransforms) { // add single transform
    auto data = new Data(OSP_AFFINE3F, vec3ul(1));
    auto &dataA3f = data->as<affine3f>();
    *dataA3f.data() = getParam<affine3f>(
        "transform", getParam<affine3f>("xfm", affine3f(one)));
    motionTransforms = &dataA3f;
    data->refDec();
  }
  motionBlur = motionTransforms->size() > 1;
  if (motionBlur)
    time = getParam<range1f>("time", range1f(0.0f, 1.0f));
}

} // namespace ospray
