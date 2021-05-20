// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospray/OSPEnums.h"

#ifndef ISPC
#include <array>
#include <string>

namespace ospray {
#endif

typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  ISPC_MSG_SPHERES,
  ISPC_MSG_SHADING_CONTEXT,
  ISPC_MSG_MAX
} ISPCMsgId;

#ifdef ISPC
extern "C" void postStatusMsg(uniform ISPCMsgId msgId,
    uniform OSPLogLevel postAtLogLevel = OSP_LOG_DEBUG);
#else
static std::array<std::string, ISPC_MSG_MAX> ispcMessages = {
    /* ISPC_MSG_SPHERES */
    "#osp:Spheres_getAreas: Non-uniform scaling in instance "
    "transformation detected! Importance sampling for emissive "
    "materials and thus resulting image may be wrong.\n",

    /* ISPC_MSG_SHADING_CONTEXT */
    "#osp:PT: out of memory for ShadingContext allocation!\n"};

} // namespace ospray
#endif
