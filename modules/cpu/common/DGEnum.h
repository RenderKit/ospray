// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ospray {
#endif // __cplusplus

typedef enum
{
  DG_FACEFORWARD = (1 << 0), // face-forward normals
  DG_NORMALIZE = (1 << 1), // normalize normals
  DG_NG = (1 << 2), // need geometry normal Ng
  DG_NS = (1 << 3), // need shading normal Ns
  DG_COLOR = (1 << 5), // hack for now - need interpolated vertex color
  DG_TEXCOORD = (1 << 6), // calculate texture coords st
  DG_TANGENTS = (1 << 7), // calculate tangents, i.e. the partial derivatives of
                          // position wrt. texture coordinates
  DG_MOTIONBLUR = (1 << 8), // calculate interpolated transformations for MB
} DG_PostIntersectFlags;

#ifdef __cplusplus
} // namespace ospray
#endif // __cplusplus
