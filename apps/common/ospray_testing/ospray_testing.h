// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospray/ospray_cpp.h"

#include "ospray_testing_export.h"

namespace ospray {
namespace testing {

using namespace ospcommon;
using namespace ospcommon::math;

using SceneBuilderHandle = void *;

OSPRAY_TESTING_EXPORT
SceneBuilderHandle newBuilder(const std::string &type);

template <typename T>
void setParam(SceneBuilderHandle b, const std::string &type, const T &val);

OSPRAY_TESTING_EXPORT
cpp::Group buildGroup(SceneBuilderHandle b);

OSPRAY_TESTING_EXPORT
cpp::World buildWorld(SceneBuilderHandle b);

OSPRAY_TESTING_EXPORT
void commit(SceneBuilderHandle b);

OSPRAY_TESTING_EXPORT
void release(SceneBuilderHandle b);

} // namespace testing
} // namespace ospray

#include "detail/ospray_testing.inl"
