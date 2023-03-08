// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// We don't want an instance of ObjectFactory static table in this library
// so we have to include it with import define so the table will be imported
// from 'ospray' library
#define OBJECTFACTORY_IMPORT
#include "common/ObjectFactory.h"

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
