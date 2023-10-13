// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Group;

struct Instance
{
  Group *group;
  RTCGeometry geom; // only to access rtcGetGeometryTransform

  AffineSpace3f xfm;
  AffineSpace3f rcp_xfm;
  bool motionBlur;
  // user defined id.  Typically used for picking
  uint32 userID;

#ifdef __cplusplus
  Instance()
      : group(nullptr),
        geom(nullptr),
        xfm(one),
        rcp_xfm(one),
        motionBlur(false),
        userID(-1u)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
