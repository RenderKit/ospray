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

// ospray
#include "common/Managed.h"

namespace ospray {

  struct QueryableTask : public ManagedObject
  {
    QueryableTask()           = default;
    ~QueryableTask() override = default;

    virtual bool isFinished(OSPSyncEvent = OSP_TASK_FINISHED) = 0;

    virtual void wait(OSPSyncEvent) = 0;
    virtual void cancel()           = 0;
    virtual float getProgress()     = 0;
  };

}  // namespace ospray
