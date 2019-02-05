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

#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/memory/malloc.h"
#include <vector>
#include <assert.h>

namespace ospcommon
{
#define __dllexport /**/

  /* Signature of ispc-generated 'task' functions */
  using ISPCTaskFunc = void (*)(void* data,
                                int threadIndex,
                                int threadCount,
                                int taskIndex,
                                int taskCount);

  extern "C" __dllexport void* ISPCAlloc(void** taskPtr, int64_t size, int32_t alignment)
  {
    if (*taskPtr == nullptr) *taskPtr = new std::vector<void*>;
    std::vector<void*>* lst = (std::vector<void*>*)(*taskPtr);
    void* ptr = memory::alignedMalloc((size_t)size,alignment);
    lst->push_back(ptr);
    return ptr;
  }

  extern "C" __dllexport void ISPCSync(void* task)
  {
    std::vector<void*>* lst = (std::vector<void*>*)task;
    for (size_t i=0; i<lst->size(); i++) memory::alignedFree((*lst)[i]);
    delete lst;
  }

  extern "C" __dllexport void ISPCLaunch(void** /*taskPtr*/,
                                         void* func,
                                         void* data,
                                         int count)
  {
    tasking::parallel_for(count,[&] (const int i) {
      const int threadIndex = i; //(int) TaskScheduler::threadIndex();
      const int threadCount = count; //(int) TaskScheduler::threadCount();
      ((ISPCTaskFunc)func)(data,threadIndex,threadCount,i,count);
    });
  }
}
