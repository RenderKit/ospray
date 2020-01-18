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

#include "ManagedObject.h"

namespace ospray {
  namespace cpp {

    class Future : public ManagedObject<OSPFuture, OSP_FUTURE>
    {
     public:
      Future(const Future &copy);
      Future(OSPFuture existing = nullptr);

      bool isReady(OSPSyncEvent = OSP_TASK_FINISHED);
      void wait(OSPSyncEvent = OSP_TASK_FINISHED);
      void cancel();
      float progress();
    };

    static_assert(sizeof(Future) == sizeof(OSPFuture),
                  "cpp::Future can't have data members!");

    // Inlined function definitions ///////////////////////////////////////////

    inline Future::Future(const Future &copy)
        : ManagedObject<OSPFuture, OSP_FUTURE>(copy.handle())
    {
      ospRetain(copy.handle());
    }

    inline Future::Future(OSPFuture existing)
        : ManagedObject<OSPFuture, OSP_FUTURE>(existing)
    {
    }

    inline bool Future::isReady(OSPSyncEvent e)
    {
      return ospIsReady(handle(), e);
    }

    inline void Future::wait(OSPSyncEvent e)
    {
      ospWait(handle(), e);
    }

    inline void Future::cancel()
    {
      ospCancel(handle());
    }

    inline float Future::progress()
    {
      return ospGetProgress(handle());
    }

  }  // namespace cpp

  OSPTYPEFOR_SPECIALIZATION(cpp::Future, OSP_FUTURE);

}  // namespace ospray
