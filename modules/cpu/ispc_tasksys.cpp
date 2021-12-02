// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <assert.h>
#include <vector>
#include "rkcommon/memory/malloc.h"
#include "rkcommon/tasking/parallel_for.h"

namespace rkcommon {
#define __dllexport /**/

/* Signature of ispc-generated 'task' functions */
using ISPCTaskFunc = void (*)(void *data,
    int threadIndex,
    int threadCount,
    int taskIndex,
    int taskCount,
    int taskIndex0,
    int taskIndex1,
    int taskIndex2,
    int taskCount0,
    int taskCount1,
    int taskCount2);

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

extern "C" __dllexport void ISPCLaunch(void ** /*taskPtr*/,
    void *func,
    void *data,
    int taskCount0,
    int taskCount1,
    int taskCount2)
{
  const size_t nTasks =
      size_t(taskCount0) * size_t(taskCount1) * size_t(taskCount2);
  tasking::parallel_for(nTasks, [&](const size_t i) {
    const int taskIndex0 = i % taskCount0;
    const int taskIndex1 = (i / taskCount0) % taskCount1;
    const int taskIndex2 = i / (taskCount0 * taskCount1);
    ((ISPCTaskFunc)func)(data,
        i,
        nTasks,
        i,
        nTasks,
        taskIndex0,
        taskIndex1,
        taskIndex2,
        taskCount0,
        taskCount1,
        taskCount2);
  });
}
} // namespace rkcommon
