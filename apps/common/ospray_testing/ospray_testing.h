// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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

  }  // namespace testing
}  // namespace ospray

#include "detail/ospray_testing.inl"
