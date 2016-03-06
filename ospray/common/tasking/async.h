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

template<typename Task>
inline void async(const Task& fcn)
{
#ifdef OSPRAY_USE_TBB
  struct LocalTBBTask : public tbb::task
  {
    Task func;
    tbb::task* execute() override
    {
      func();
      return nullptr;
    }

    LocalTBBTask( const Task& f ) : func(f) {}
  };

  tbb::task::enqueue(*new(tbb::task::allocate_root())LocalTBBTask(fcn));
#elif defined(OSPRAY_USE_CILK)
  cilk_spawn fcn();
#else// OpenMP --> synchronous!
  fcn();
#endif
}

}//namespace ospray
