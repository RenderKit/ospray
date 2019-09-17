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

// ospcommon
#include "ospcommon/tasking/AsyncTask.h"
// ospray
#include "common/Future.h"
#include "fb/FrameBuffer.h"

namespace ospray {
  namespace mpi {

    //! Dummy task to satisfy the public API OSPFuture requirements, but
    //  assumes the frame is already rendered.
    struct SynchronousRenderTask : public Future
    {
      SynchronousRenderTask(FrameBuffer *);
      ~SynchronousRenderTask() override = default;

      bool isFinished(OSPSyncEvent event) override;
      void wait(OSPSyncEvent event) override;
      void cancel() override;
      float getProgress() override;

     private:
      Ref<FrameBuffer> fb;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline SynchronousRenderTask::SynchronousRenderTask(FrameBuffer *_fb)
        : fb(_fb)
    {
    }

    inline bool SynchronousRenderTask::isFinished(OSPSyncEvent /*event*/)
    {
      return true;
    }

    inline void SynchronousRenderTask::wait(OSPSyncEvent)
    {
      // no-op
    }

    inline void SynchronousRenderTask::cancel()
    {
      // no-op
    }

    inline float SynchronousRenderTask::getProgress()
    {
      return 1.f;
    }

  }  // namespace mpi
}  // namespace ospray
