// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// rkcommon
#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
// ospray
#include "../Traits.h"

namespace ospray {

OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2uc, OSP_VEC2UC);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3uc, OSP_VEC3UC);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4uc, OSP_VEC4UC);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2c, OSP_VEC2C);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3c, OSP_VEC3C);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4c, OSP_VEC4C);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2us, OSP_VEC2US);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3us, OSP_VEC3US);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4us, OSP_VEC4US);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2s, OSP_VEC2S);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3s, OSP_VEC3S);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4s, OSP_VEC4S);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2i, OSP_VEC2I);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3i, OSP_VEC3I);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4i, OSP_VEC4I);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2ui, OSP_VEC2UI);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3ui, OSP_VEC3UI);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4ui, OSP_VEC4UI);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2l, OSP_VEC2L);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3l, OSP_VEC3L);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4l, OSP_VEC4L);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2ul, OSP_VEC2UL);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3ul, OSP_VEC3UL);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4ul, OSP_VEC4UL);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2f, OSP_VEC2F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3f, OSP_VEC3F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4f, OSP_VEC4F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec2d, OSP_VEC2D);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec3d, OSP_VEC3D);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::vec4d, OSP_VEC4D);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::box1i, OSP_BOX1I);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::box2i, OSP_BOX2I);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::box3i, OSP_BOX3I);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::box4i, OSP_BOX4I);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::box1f, OSP_BOX1F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::box2f, OSP_BOX2F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::box3f, OSP_BOX3F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::box4f, OSP_BOX4F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::linear2f, OSP_LINEAR2F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::linear3f, OSP_LINEAR3F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::affine2f, OSP_AFFINE2F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::affine3f, OSP_AFFINE3F);
OSPTYPEFOR_SPECIALIZATION(rkcommon::math::quatf, OSP_QUATF);

#ifdef OSPRAY_RKCOMMON_DEFINITIONS
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2uc);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3uc);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4uc);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2c);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3c);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4c);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2us);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3us);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4us);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2s);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3s);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4s);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2i);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3i);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4i);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2ui);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3ui);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4ui);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2l);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3l);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4l);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2ul);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3ul);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4ul);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2f);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3f);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4f);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec2d);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec3d);
OSPTYPEFOR_DEFINITION(rkcommon::math::vec4d);
OSPTYPEFOR_DEFINITION(rkcommon::math::box1i);
OSPTYPEFOR_DEFINITION(rkcommon::math::box2i);
OSPTYPEFOR_DEFINITION(rkcommon::math::box3i);
OSPTYPEFOR_DEFINITION(rkcommon::math::box4i);
OSPTYPEFOR_DEFINITION(rkcommon::math::box1f);
OSPTYPEFOR_DEFINITION(rkcommon::math::box2f);
OSPTYPEFOR_DEFINITION(rkcommon::math::box3f);
OSPTYPEFOR_DEFINITION(rkcommon::math::box4f);
OSPTYPEFOR_DEFINITION(rkcommon::math::linear2f);
OSPTYPEFOR_DEFINITION(rkcommon::math::linear3f);
OSPTYPEFOR_DEFINITION(rkcommon::math::affine2f);
OSPTYPEFOR_DEFINITION(rkcommon::math::affine3f);
OSPTYPEFOR_DEFINITION(rkcommon::math::quatf);
#endif

} // namespace ospray
