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
#include "transferFunction/TransferFunction.h"
// ospcommon
#include "ospcommon/math/box.h"
#include "ospcommon/utility/OnScopeExit.h"
// ospray
#include "ospray/ospray_util.h"

using namespace ospcommon;
using namespace ospcommon::math;

extern "C" OSPTransferFunction ospTestingNewTransferFunction(osp_vec2f range,
                                                             const char *name)
{
  auto tfnCreator =
      ospray::testing::objectFactory<ospray::testing::TransferFunction>(
          "testing_transfer_function", name);

  utility::OnScopeExit cleanup([=]() { delete tfnCreator; });

  if (tfnCreator != nullptr)
    return tfnCreator->createTransferFunction(range);
  else
    return {};
}

//////////////////////////////////////////////////////////////////////////////
// New C++ interface /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

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
