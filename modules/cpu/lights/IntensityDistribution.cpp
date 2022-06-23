// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "IntensityDistribution.h"

namespace ospray {

void IntensityDistribution::readParams(ManagedObject &obj)
{
  c0 = obj.getParam<vec3f>("c0", c0);
  lid = obj.getParamDataT<float, 2>("intensityDistribution");
  if (lid) {
    if (lid->numItems.z > 1)
      throw std::runtime_error(obj.toString()
          + " must have (at most 2D) 'intensityDistribution' array using the first two dimensions.");
    size = vec2i(lid->numItems.x, lid->numItems.y);
    if (size.x < 2)
      throw std::runtime_error(obj.toString()
          + " 'intensityDistribution' must have data for at least two gamma angles.");
    if (!lid->compact()) {
      postStatusMsg(OSP_LOG_WARNING)
          << obj.toString()
          << " does currently not support strides for 'intensityDistribution', copying data.";

      const auto data = new Data(OSP_FLOAT, lid->numItems);
      data->copy(*lid, vec3ui(0));
      lid = &(data->as<float, 2>());
      data->refDec();
    }
  }
}

} // namespace ospray
