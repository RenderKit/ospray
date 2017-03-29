// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "TaskingTypeTraits.h"
#include "schedule.inl"

namespace ospcommon {

  // NOTE(jda) - This abstraction takes a lambda which should take captured
  //             variables by *value* to ensure no captured references race
  //             with the task itself.

  // NOTE(jda) - No priority is associated with this call, but could be added
  //             later with a hint enum, using a default value for the priority
  //             to not require specifying it.
  template<typename TASK_T>
  inline void schedule(TASK_T&& fcn)
  {
    static_assert(has_operator_method<TASK_T>::value,
                  "ospcommon::schedule() requires the implementation of method "
                  "'void TASK_T::operator()'.");

    schedule_impl(std::forward<TASK_T>(fcn));
  }

} // ::ospcommon
