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

#include "Managed.h"

#include <future>

namespace ospray {

  template <typename T>
  struct Future : public ManagedObject
  {
    ~Future() override = default;

    T get();

   private:
    std::future<T> result;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  template <typename T>
  T Future<T>::get()
  {
    return result.get();
  }

}  // namespace ospray
