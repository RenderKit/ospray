// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#ifdef OSPRAY_USE_TBB
#  include <tbb/task.h>
#elif defined(OSPRAY_USE_CILK)
#  include <cilk/cilk.h>
#endif

namespace ospray {

// NOTE(jda) - This abstraction takes a lambda which should take captured
//             variables by *value* to ensure no captured references race
//             with the task itself.

// NOTE(jda) - No priority is associated with this call, but could be added
//             later with a hint enum, using a default value for the priority
//             to not require specifying it.
template<typename TASK>
inline void async(const TASK& fcn)
{
#ifdef OSPRAY_USE_TBB
  struct LocalTBBTask : public tbb::task
  {
    TASK func;
    tbb::task* execute() override
    {
      func();
      return nullptr;
    }

    LocalTBBTask( const TASK& f ) : func(f) {}
  };

  tbb::task::enqueue(*new(tbb::task::allocate_root())LocalTBBTask(fcn));
#elif defined(OSPRAY_USE_CILK)
  cilk_spawn fcn();
#else// OpenMP or Debug --> synchronous!
  fcn();
#endif
}

}//namespace ospray
