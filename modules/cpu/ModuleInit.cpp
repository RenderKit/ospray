// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// We don't want an instance of ObjectFactory static table in this library
// so we have to include it with import define so the table will be imported
// from 'ospray' library
#define OBJECTFACTORY_IMPORT
#include "common/ObjectFactory.h"

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
