// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <vector>
#include "ospcommon/memory/malloc.h"
#include "ospcommon/tasking/parallel_for.h"

namespace ospcommon {
#define __dllexport /**/

/* Signature of ispc-generated 'task' functions */
using ISPCTaskFunc = void (*)(
    void *data, int threadIndex, int threadCount, int taskIndex, int taskCount);

extern "C" __dllexport void *ISPCAlloc(
    void **taskPtr, int64_t size, int32_t alignment)
{
  if (*taskPtr == nullptr)
    *taskPtr = new std::vector<void *>;
  std::vector<void *> *lst = (std::vector<void *> *)(*taskPtr);
  void *ptr = memory::alignedMalloc((size_t)size, alignment);
  lst->push_back(ptr);
  return ptr;
}

extern "C" __dllexport void ISPCSync(void *task)
{
  std::vector<void *> *lst = (std::vector<void *> *)task;
  for (size_t i = 0; i < lst->size(); i++)
    memory::alignedFree((*lst)[i]);
  delete lst;
}

extern "C" __dllexport void ISPCLaunch(
    void ** /*taskPtr*/, void *func, void *data, int count)
{
  tasking::parallel_for(count, [&](const int i) {
    const int threadIndex = i; //(int) TaskScheduler::threadIndex();
    const int threadCount = count; //(int) TaskScheduler::threadCount();
    ((ISPCTaskFunc)func)(data, threadIndex, threadCount, i, count);
  });
}
} // namespace ospcommon
