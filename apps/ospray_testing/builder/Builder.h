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

#include "../ospray_testing.h"
// ospcommon
#include "ospcommon/memory/IntrusivePtr.h"
#include "ospcommon/utility/ParameterizedObject.h"

namespace ospray {
  namespace testing {
    namespace detail {

      struct Builder : public memory::RefCountedObject,
                       public utility::ParameterizedObject
      {
        virtual ~Builder() = default;

        virtual SceneBuilderHandle createBuilder() const = 0;
      };

    }  // namespace detail
  }    // namespace testing
}  // namespace ospray

#define OSP_REGISTER_TESTING_BUILDER(InternalClassName, Name)          \
  extern "C" OSPRAY_TESTING_DLLEXPORT ospray::testing::detail::Builder \
      *ospray_create_testing_builder__##Name()                         \
  {                                                                    \
    return new InternalClassName;                                      \
  }                                                                    \
  /* Extra declaration to avoid "extra ;" pedantic warnings */         \
  ospray::testing::detail::Builder *ospray_create_testing_builder__##Name()
