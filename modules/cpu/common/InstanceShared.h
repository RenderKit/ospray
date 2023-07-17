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

  // Scene and geomID are used to call rtcGetGeometryTransformFromScene only
  RTCScene scene;
  unsigned int geomID;

  AffineSpace3f xfm;
  AffineSpace3f rcp_xfm;
  bool motionBlur;
  // user defined id.  Typically used for picking
  uint32 userID;

#ifdef __cplusplus
  Instance()
      : group(nullptr),
        scene(nullptr),
        geomID(0),
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
