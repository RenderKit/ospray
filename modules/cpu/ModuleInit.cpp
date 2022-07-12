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

#include "ISPCDevice.h"
#include "OSPCommon.h"
#include "camera/registration.h"
#include "fb/registration.h"
#include "geometry/registration.h"
#include "lights/registration.h"
#include "render/registration.h"
#include "texture/registration.h"
#include "volume/transferFunction/registration.h"

using namespace ospray;

extern "C" OSPError OSPRAY_DLLEXPORT ospray_module_init_cpu(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  auto status = moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR) {
    api::Device::registerType<api::ISPCDevice>("cpu");

    registerAllCameras();
    registerAllImageOps();
    registerAllGeometries();
    registerAllLights();
    registerAllMaterials();
    registerAllRenderers();
    registerAllTextures();
    registerAllTransferFunctions();
  }

  return status;
}
