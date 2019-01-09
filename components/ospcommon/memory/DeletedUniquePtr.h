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

#include <functional>
#include <memory>
#include <utility>

namespace ospcommon {
  namespace memory {

    template <typename T>
    using DeletedUniquePtr = std::unique_ptr<T, std::function<void(T*)>>;

    template <typename T, typename DELETE_FCN, typename ...Args>
    inline DeletedUniquePtr<T> make_deleted_unique(DELETE_FCN&& deleter,
                                                   Args&& ...args)
    {
      return DeletedUniquePtr<T>(new T(std::forward<Args>(args)...), deleter);
    }

  } // ::ospcommon::utility
} // ::ospcommon
