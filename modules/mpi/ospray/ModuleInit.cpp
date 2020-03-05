// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/OSPCommon.h"

extern "C" OSPError ospray_module_init_mpi(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  return ospray::moduleVersionCheck(versionMajor, versionMinor);
}
