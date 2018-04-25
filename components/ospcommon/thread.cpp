// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "thread.h"
#include "sysinfo.h"

#include "intrinsics.h"

#include <iostream>
#include <xmmintrin.h>

#ifdef PTHREADS_WIN32
#pragma comment (lib, "pthreadVC.lib")
#endif

////////////////////////////////////////////////////////////////////////////////
/// Windows Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ospcommon
{
  /*! set the affinity of a given thread */
  void setAffinity(HANDLE thread, ssize_t affinity)
  {
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    typedef WORD (WINAPI *GetActiveProcessorGroupCountFunc)();
    typedef DWORD (WINAPI *GetActiveProcessorCountFunc)(WORD);
    typedef BOOL (WINAPI *SetThreadGroupAffinityFunc)(HANDLE, const GROUP_AFFINITY *, PGROUP_AFFINITY);
    typedef BOOL (WINAPI *SetThreadIdealProcessorExFunc)(HANDLE, PPROCESSOR_NUMBER, PPROCESSOR_NUMBER);
    HMODULE hlib = LoadLibrary("Kernel32");
    GetActiveProcessorGroupCountFunc pGetActiveProcessorGroupCount = (GetActiveProcessorGroupCountFunc)GetProcAddress(hlib, "GetActiveProcessorGroupCount");
    GetActiveProcessorCountFunc pGetActiveProcessorCount = (GetActiveProcessorCountFunc)GetProcAddress(hlib, "GetActiveProcessorCount");
    SetThreadGroupAffinityFunc pSetThreadGroupAffinity = (SetThreadGroupAffinityFunc)GetProcAddress(hlib, "SetThreadGroupAffinity");
    SetThreadIdealProcessorExFunc pSetThreadIdealProcessorEx = (SetThreadIdealProcessorExFunc)GetProcAddress(hlib, "SetThreadIdealProcessorEx");
    if (pGetActiveProcessorGroupCount && pGetActiveProcessorCount && pSetThreadGroupAffinity && pSetThreadIdealProcessorEx &&
       ((osvi.dwMajorVersion > 6) || ((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion >= 1))))
    {
      int groups = pGetActiveProcessorGroupCount();
      int totalProcessors = 0, group = 0, number = 0;
      for (int i = 0; i<groups; i++) {
        int processors = pGetActiveProcessorCount(i);
        if (totalProcessors + processors > affinity) {
          group = i;
          number = (int)affinity - totalProcessors;
          break;
        }
        totalProcessors += processors;
      }

      GROUP_AFFINITY groupAffinity;
      groupAffinity.Group = (WORD)group;
      groupAffinity.Mask = (KAFFINITY)(uint64_t(1) << number);
      groupAffinity.Reserved[0] = 0;
      groupAffinity.Reserved[1] = 0;
      groupAffinity.Reserved[2] = 0;
      if (!pSetThreadGroupAffinity(thread, &groupAffinity, nullptr))
        WARNING("SetThreadGroupAffinity failed"); // on purpose only a warning

      PROCESSOR_NUMBER processorNumber;
      processorNumber.Group = group;
      processorNumber.Number = number;
      processorNumber.Reserved = 0;
      if (!pSetThreadIdealProcessorEx(thread, &processorNumber, nullptr))
        WARNING("SetThreadIdealProcessorEx failed"); // on purpose only a warning
    }
    else
    {
      if (!SetThreadAffinityMask(thread, DWORD_PTR(uint64_t(1) << affinity)))
        WARNING("SetThreadAffinityMask failed"); // on purpose only a warning
      if (SetThreadIdealProcessor(thread, (DWORD)affinity) == (DWORD)-1)
        WARNING("SetThreadIdealProcessor failed"); // on purpose only a warning
      }
  }

  /*! set affinity of the calling thread */
  void setAffinity(ssize_t affinity) {
    setAffinity(GetCurrentThread(), affinity);
  }

  struct ThreadStartupData
  {
  public:
    ThreadStartupData (thread_func f, void* arg)
      : f(f), arg(arg) {}
  public:
    thread_func f;
    void* arg;
  };

  static void* threadStartup(ThreadStartupData* parg)
  {
    _mm_setcsr(_mm_getcsr() | /*FTZ:*/ (1<<15) | /*DAZ:*/ (1<<6));
    parg->f(parg->arg);
    delete parg;
    return nullptr;
  }

#if !defined(PTHREADS_WIN32)

  /*! creates a hardware thread running on specific core */
  thread_t createThread(thread_func f, void* arg, size_t stack_size, ssize_t threadID)
  {
    HANDLE thread = CreateThread(nullptr, stack_size, (LPTHREAD_START_ROUTINE)threadStartup, new ThreadStartupData(f,arg), 0, nullptr);
    if (thread == nullptr) throw std::runtime_error("ospcommon::CreateThread failed");
    if (threadID >= 0) setAffinity(thread, threadID);
    return thread_t(thread);
  }

  /*! the thread calling this function gets yielded */
  void yield() {
    SwitchToThread();
  }

  /*! waits until the given thread has terminated */
  void join(thread_t tid) {
    WaitForSingleObject(HANDLE(tid), INFINITE);
    CloseHandle(HANDLE(tid));
  }

  /*! destroy a hardware thread by its handle */
  void destroyThread(thread_t tid) {
    TerminateThread(HANDLE(tid),0);
    CloseHandle(HANDLE(tid));
  }

#endif
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// Linux Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __linux__
namespace ospcommon
{
  /*! set affinity of the calling thread */
  void setAffinity(ssize_t affinity)
  {
    cpu_set_t cset;
    CPU_ZERO(&cset);
    CPU_SET(affinity, &cset);

    if (pthread_setaffinity_np(pthread_self(), sizeof(cset), &cset) != 0)
      WARNING("pthread_setaffinity_np failed"); // on purpose only a warning
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// MacOSX Platform
////////////////////////////////////////////////////////////////////////////////

#ifdef __APPLE__

#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#include <mach/mach_init.h>

namespace ospcommon
{
  /*! set affinity of the calling thread */
  void setAffinity(ssize_t affinity)
  {
    thread_affinity_policy ap;
    ap.affinity_tag = affinity;
    if (thread_policy_set(mach_thread_self(),THREAD_AFFINITY_POLICY,(thread_policy_t)&ap,THREAD_AFFINITY_POLICY_COUNT) != KERN_SUCCESS)
      WARNING("setting thread affinity failed"); // on purpose only a warning
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// Unix Platform
////////////////////////////////////////////////////////////////////////////////

#if !defined(_WIN32) || defined(PTHREADS_WIN32)

#include <pthread.h>
#include <sched.h>

namespace ospcommon
{
  struct ThreadStartupData
  {
  public:
    ThreadStartupData (thread_func _f, void* _arg, int _affinity)
      : f(_f), arg(_arg), affinity(_affinity) {}
  public:
    thread_func f;
    void* arg;
    ssize_t affinity;
  };

  static void* threadStartup(ThreadStartupData* parg)
  {
    _mm_setcsr(_mm_getcsr() | /*FTZ:*/ (1<<15) | /*DAZ:*/ (1<<6));

#if !defined(__linux__)
    if (parg->affinity >= 0)
	setAffinity(parg->affinity);
#endif

    parg->f(parg->arg);
    delete parg;
    return nullptr;
  }

  /*! creates a hardware thread running on specific core */
  thread_t createThread(thread_func f, void* arg, size_t stack_size, ssize_t threadID)
  {
    /* set stack size */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stack_size > 0) pthread_attr_setstacksize (&attr, stack_size);

    /* create thread */
    pthread_t* tid = new pthread_t;
    if (pthread_create(tid,&attr,(void*(*)(void*))threadStartup,new ThreadStartupData(f,arg,threadID)) != 0)
      throw std::runtime_error("ospcommon::pthread_create failed");

    /* set affinity */
#ifdef __linux__
    if (threadID >= 0) {
      cpu_set_t cset;
      CPU_ZERO(&cset);
      CPU_SET(threadID, &cset);
      if (pthread_setaffinity_np(*tid,sizeof(cpu_set_t),&cset))
        WARNING("pthread_setaffinity_np failed"); // on purpose only a warning
    }
#endif

    return thread_t(tid);
  }

  /*! the thread calling this function gets yielded */
  void yield() {
    sched_yield();
  }

  /*! waits until the given thread has terminated */
  void join(thread_t tid) {
    if (pthread_join(*(pthread_t*)tid, nullptr) != 0)
      throw std::runtime_error("ospcommon::pthread_join failed");
    delete (pthread_t*)tid;
  }

  /*! destroy a hardware thread by its handle */
  void destroyThread(thread_t tid) {
    pthread_cancel(*(pthread_t*)tid);
    delete (pthread_t*)tid;
  }
}

#endif

namespace ospcommon {
  void ospray_Thread_runThread(void *arg)
  {
    Thread *t = (Thread *)arg;
    if (t->desiredThreadID >= 0) {
      printf("pinning to thread %i\n",t->desiredThreadID);
      ospcommon::setAffinity(t->desiredThreadID);
    }

    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    t->run();
  }

  void Thread::join()
  {
    ospcommon::join(tid);
  }

  /*!  start thread execution */
  void Thread::start(int threadID)
  {
    desiredThreadID = threadID;
    this->tid = ospcommon::createThread(&ospray_Thread_runThread,this);
  }
}// namespace ospcommon
