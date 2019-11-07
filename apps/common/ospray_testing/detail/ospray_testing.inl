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

#include "../builders/Builder.h"

namespace ospray {
  namespace testing {

    template <typename T>
    inline void setParam(SceneBuilderHandle _b,
                         const std::string &type,
                         const T &val)
    {
      auto *b = (detail::Builder *)_b;
      b->setParam(type, val);
    }

  }  // namespace testing
}  // namespace ospray
