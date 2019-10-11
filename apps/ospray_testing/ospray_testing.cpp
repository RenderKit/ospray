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

// ospray_testing
#include "ospray_testing.h"
#include "detail/objectFactory.h"

using namespace ospcommon;
using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    SceneBuilderHandle newBuilder(const std::string &type)
    {
      auto *b = objectFactory<detail::Builder>("testing_builder", type);
      return (SceneBuilderHandle)b;
    }

    cpp::Group buildGroup(SceneBuilderHandle _b)
    {
      auto *b = (detail::Builder *)_b;
      return b->buildGroup();
    }

    cpp::World buildWorld(SceneBuilderHandle _b)
    {
      auto *b = (detail::Builder *)_b;
      return b->buildWorld();
    }

    void commit(SceneBuilderHandle _b)
    {
      auto *b = (detail::Builder *)_b;
      b->commit();
    }

    void release(SceneBuilderHandle _b)
    {
      auto *b = (detail::Builder *)_b;
      b->refDec();
    }

  }  // namespace testing
}  // namespace ospray
