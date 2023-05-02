// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/RendererShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

enum DebugRendererType
{
  TEST_FRAME,
  RAY_DIR,
  EYE_LIGHT,
  NG,
  NS,
  COLOR,
  TEX_COORD,
  DPDS,
  DPDT,
  PRIM_ID,
  GEOM_ID,
  INST_ID,
  BACKFACING_NG,
  BACKFACING_NS,
  VOLUME
};

struct DebugRenderer
{
  Renderer super;
  DebugRendererType type;

#ifdef __cplusplus
  DebugRenderer() : type(TEST_FRAME) {}
#endif
};
#ifdef __cplusplus
}

#ifdef OSPRAY_TARGET_SYCL
inline constexpr sycl::specialization_id<ispc::DebugRendererType>
    debugRendererType;
#endif

#endif
