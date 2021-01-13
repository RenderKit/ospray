// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>
// ospray_cpp
#include "ospray/ospray_cpp.h"
#include "ospray/ospray_cpp/ext/rkcommon.h"
#define OSPRAY_GLM_DEFINITIONS
#include "ospray/ospray_cpp/ext/glm.h"
// glm extras
#include "glm/gtc/matrix_transform.hpp"

TEST(glm, TypeCompatibility)
{
  static_assert(sizeof(rkcommon::math::vec2f) == sizeof(glm::vec2), "");
  static_assert(sizeof(rkcommon::math::vec3f) == sizeof(glm::vec3), "");
  static_assert(sizeof(rkcommon::math::vec4f) == sizeof(glm::vec4), "");
  static_assert(sizeof(rkcommon::math::vec2i) == sizeof(glm::ivec2), "");
  static_assert(sizeof(rkcommon::math::vec3i) == sizeof(glm::ivec3), "");
  static_assert(sizeof(rkcommon::math::vec4i) == sizeof(glm::ivec4), "");
  static_assert(sizeof(rkcommon::math::vec2ui) == sizeof(glm::uvec2), "");
  static_assert(sizeof(rkcommon::math::vec3ui) == sizeof(glm::uvec3), "");
  static_assert(sizeof(rkcommon::math::vec4ui) == sizeof(glm::uvec4), "");
  static_assert(sizeof(rkcommon::math::linear2f) == sizeof(glm::mat2x2), "");
  static_assert(sizeof(rkcommon::math::linear3f) == sizeof(glm::mat3x3), "");
  static_assert(sizeof(rkcommon::math::affine2f) == sizeof(glm::mat3x2), "");
  static_assert(sizeof(rkcommon::math::affine3f) == sizeof(glm::mat4x3), "");

  // Test vecXf //

  glm::vec2 glm_vec2(1, 2);
  rkcommon::math::vec2f rk_vec2f;
  std::memcpy(&rk_vec2f, &glm_vec2, sizeof(rk_vec2f));
  ASSERT_EQ(rk_vec2f.x, 1);
  ASSERT_EQ(rk_vec2f.y, 2);

  glm::vec3 glm_vec3(1, 2, 3);
  rkcommon::math::vec3f rk_vec3f;
  std::memcpy(&rk_vec3f, &glm_vec3, sizeof(rk_vec3f));
  ASSERT_EQ(rk_vec3f.x, 1);
  ASSERT_EQ(rk_vec3f.y, 2);
  ASSERT_EQ(rk_vec3f.z, 3);

  glm::vec4 glm_vec4(1, 2, 3, 4);
  rkcommon::math::vec4f rk_vec4f;
  std::memcpy(&rk_vec4f, &glm_vec4, sizeof(rk_vec4f));
  ASSERT_EQ(rk_vec4f.x, 1);
  ASSERT_EQ(rk_vec4f.y, 2);
  ASSERT_EQ(rk_vec4f.z, 3);
  ASSERT_EQ(rk_vec4f.w, 4);

  // Test vecXi //

  glm::ivec2 glm_ivec2(1, 2);
  rkcommon::math::vec2i rk_vec2i;
  std::memcpy(&rk_vec2i, &glm_ivec2, sizeof(rk_vec2i));
  ASSERT_EQ(rk_vec2i.x, 1);
  ASSERT_EQ(rk_vec2i.y, 2);

  glm::ivec3 glm_ivec3(1, 2, 3);
  rkcommon::math::vec3i rk_vec3i;
  std::memcpy(&rk_vec3i, &glm_ivec3, sizeof(rk_vec3i));
  ASSERT_EQ(rk_vec3i.x, 1);
  ASSERT_EQ(rk_vec3i.y, 2);
  ASSERT_EQ(rk_vec3i.z, 3);

  glm::ivec4 glm_ivec4(1, 2, 3, 4);
  rkcommon::math::vec4i rk_vec4i;
  std::memcpy(&rk_vec4i, &glm_ivec4, sizeof(rk_vec4i));
  ASSERT_EQ(rk_vec4i.x, 1);
  ASSERT_EQ(rk_vec4i.y, 2);
  ASSERT_EQ(rk_vec4i.z, 3);
  ASSERT_EQ(rk_vec4i.w, 4);

  // Test vecXu //

  glm::uvec2 glm_uvec2(1, 2);
  rkcommon::math::vec2ui rk_vec2ui;
  std::memcpy(&rk_vec2ui, &glm_uvec2, sizeof(rk_vec2ui));
  ASSERT_EQ(rk_vec2ui.x, 1);
  ASSERT_EQ(rk_vec2ui.y, 2);

  glm::uvec3 glm_uvec3(1, 2, 3);
  rkcommon::math::vec3ui rk_vec3ui;
  std::memcpy(&rk_vec3ui, &glm_uvec3, sizeof(rk_vec3ui));
  ASSERT_EQ(rk_vec3ui.x, 1);
  ASSERT_EQ(rk_vec3ui.y, 2);
  ASSERT_EQ(rk_vec3ui.z, 3);

  glm::uvec4 glm_uvec4(1, 2, 3, 4);
  rkcommon::math::vec4ui rk_vec4ui;
  std::memcpy(&rk_vec4ui, &glm_uvec4, sizeof(rk_vec4ui));
  ASSERT_EQ(rk_vec4ui.x, 1);
  ASSERT_EQ(rk_vec4ui.y, 2);
  ASSERT_EQ(rk_vec4ui.z, 3);
  ASSERT_EQ(rk_vec4ui.w, 4);

  // Matrix tests //

  // linear

  glm::mat2x2 glm_mat2x2(1.f);
  rkcommon::math::linear2f rk_linear2f;
  std::memcpy(&rk_linear2f, &glm_mat2x2, sizeof(rk_linear2f));

  ASSERT_EQ(rk_linear2f.vx, rkcommon::math::vec2f(1, 0));
  ASSERT_EQ(rk_linear2f.vy, rkcommon::math::vec2f(0, 1));

  glm::mat3x3 glm_mat3x3(1.f);
  rkcommon::math::linear3f rk_linear3f;
  std::memcpy(&rk_linear3f, &glm_mat3x3, sizeof(rk_linear3f));

  ASSERT_EQ(rk_linear3f.vx, rkcommon::math::vec3f(1, 0, 0));
  ASSERT_EQ(rk_linear3f.vy, rkcommon::math::vec3f(0, 1, 0));
  ASSERT_EQ(rk_linear3f.vz, rkcommon::math::vec3f(0, 0, 1));

  // affine

  glm::mat4x3 glm_mat4x3 = glm::translate(glm::mat4(1.f), glm::vec3(1, 2, 3));
  rkcommon::math::affine3f rk_affine3f;
  std::memcpy(&rk_affine3f, &glm_mat4x3, sizeof(rk_affine3f));

  ASSERT_EQ(rk_affine3f.l.vx, rkcommon::math::vec3f(1, 0, 0));
  ASSERT_EQ(rk_affine3f.l.vy, rkcommon::math::vec3f(0, 1, 0));
  ASSERT_EQ(rk_affine3f.l.vz, rkcommon::math::vec3f(0, 0, 1));
  ASSERT_EQ(rk_affine3f.p, rkcommon::math::vec3f(1, 2, 3));
}
