// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PanoramicCamera.h"
// ispc exports
#ifndef OSPRAY_TARGET_SYCL
#include "camera/PanoramicCamera_ispc.h"
#else
namespace ispc {
void *PanoramicCamera_initRay_addr();
}
#endif

namespace ospray {

PanoramicCamera::PanoramicCamera(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device, FFO_CAMERA_PANORAMIC)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.initRay = reinterpret_cast<ispc::Camera_initRay>(
      ispc::PanoramicCamera_initRay_addr());
#endif
}

std::string PanoramicCamera::toString() const
{
  return "ospray::PanoramicCamera";
}

void PanoramicCamera::commit()
{
  Camera::commit();

  getSh()->org = pos;

  if (getSh()->super.motionBlur) {
    getSh()->frame.vz = -dir;
    getSh()->frame.vy = up;
  } else {
    getSh()->frame.vz = -normalize(dir);
    getSh()->frame.vx = normalize(cross(up, getSh()->frame.vz));
    getSh()->frame.vy = cross(getSh()->frame.vz, getSh()->frame.vx);
  }

  getSh()->stereoMode = (OSPStereoMode)getParam<uint8_t>(
      "stereoMode", getParam<int32_t>("stereoMode", OSP_STEREO_NONE));
  getSh()->ipd_offset =
      0.5f * getParam<float>("interpupillaryDistance", 0.0635f);

  // flip offset to have left eye at top (image coord origin at lower left)
  if (getSh()->stereoMode == OSP_STEREO_TOP_BOTTOM)
    getSh()->ipd_offset = -getSh()->ipd_offset;
}

} // namespace ospray
