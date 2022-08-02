// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Windows specific note: The 'Device::registerType<>()' static method that
// registers 'ISPCDevice' is inlined and is using static map from the
// 'ObjectFactory' class. The map in case of 'Device' is present in the
// 'ospray' library and has to be imported here. This means that
// 'ObjectFactory' has to be declared in this translation unit with
// '__declspec(dllimport)' definition.
#ifdef _WIN32
#define OBJECTFACTORY_DLLIMPORT
#include "common/ObjectFactory.h"
#endif

#include "DenoiseFrameOp.h"
#include "common/OSPCommon.h"

extern "C" OSPError OSPRAY_DLLEXPORT ospray_module_init_denoiser(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  auto status = ospray::moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR)
    ospray::ImageOp::registerType<ospray::DenoiseFrameOp>("denoiser");

  return status;
}
