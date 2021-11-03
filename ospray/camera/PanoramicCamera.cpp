// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PanoramicCamera.h"
#include "camera/PanoramicCamera_ispc.h"

namespace ospray {

PanoramicCamera::PanoramicCamera()
{
  ispcEquivalent = ispc::PanoramicCamera_create();
}

std::string PanoramicCamera::toString() const
{
  return "ospray::PanoramicCamera";
}

void PanoramicCamera::commit()
{
  Camera::commit();

  ispc::PanoramicCamera_set(getIE(),
      (const ispc::vec3f &)pos,
      (const ispc::vec3f &)dir,
      (const ispc::vec3f &)up,
      (OSPStereoMode)getParam<uint8_t>(
          "stereoMode", getParam<int32_t>("stereoMode", OSP_STEREO_NONE)),
      getParam<float>("interpupillaryDistance", 0.0635f));
}

} // namespace ospray
