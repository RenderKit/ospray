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

/*! \file ospray/common/ProducerConsumerQueue.h \brief Abstraction for
    a mutex-protected producer-consumer queue that different threads
    can write into / pull from */

#include "ospcommon/common.h"
// stl
#include <queue>
#include <mutex>
#include <condition_variable>

namespace ospray {

  /*! \brief A (thread-safe) producer-consumer queue.

    \detailed The producer-consumer queue is intended to facilite a
    usage model where some set of "producer" threads "put()"s items
    into the queue, that a other set of thread(s) (i.e., the
    "consudmer" thread(s)) can then "get()" items from. All accesses
    are automatically thread-safe; a 'get()' on an empty queue will
    automatically put the consumer that attempted that get to sleep on
    a thread condition. */
  template<typename T>
  struct ProducerConsumerQueue {
    /*! put a new element into this queue */
    void put(T t);

    /*! put multiple new elements into this queue */
    void putSome(T *t, size_t numTs);

    /*! get element that got written into the queue */
    T get();

    /*! get elements in the queue into the given vector, and clear the queue.
      if the queue is currently empty, wait until at least one element is there */
    void getAll(std::vector<T> &all);

    /*! get elements in the queue into the given vector, and clear the queue.
      if the queue is currently empty, waiting until at least one element is there 
      */
    size_t getSome(T *some, size_t maxSize);
    /*! get elements in the queue into the given vector, and clear the queue.
      if the queue is currently empty, wait until at least one element is there 
      or until the timeOut has elapsed, in which case the function may
      return 0. */
    size_t getSomeFor(T *some, size_t maxSize, std::chrono::milliseconds timeOut);

  private:
    /*! the actual queue that holds the data */
    std::deque<T> content;
    /*! mutex to allow thread-safe access */
    std::mutex mutex;
    /*! condition that is triggered when queue is not empty. 'get'
        requests can wait on this */
    std::condition_variable notEmptyCond;
  };


  // =======================================================
  // IMPLEMENTATION SECTION
  // =======================================================


  /*! get element that got written into the queue */
  template<typename T>
  T ProducerConsumerQueue<T>::get()
  {
    std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
    notEmptyCond.wait(lock, [&]{return !content.empty();});

    T t = content.front();
    content.pop_front();
    return t;
  }

  /*! put a new element into this queue */
  template<typename T>
  void ProducerConsumerQueue<T>::put(T t)
  {
    bool wasEmpty = false;
    {
      std::lock_guard<std::mutex> lock(mutex);
      wasEmpty = content.empty();
      content.push_back(t);
    }
    if (wasEmpty)
      notEmptyCond.notify_all();
  }

  /*! put multiple new elements into this queue */
  template<typename T>
  void ProducerConsumerQueue<T>::putSome(T *t, size_t numTs)
  {
    bool wasEmpty = false;
    {
      std::lock_guard<std::mutex> lock(mutex);
      wasEmpty = content.empty();
      for (size_t i = 0; i < numTs; i++)
        content.push_back(t[i]);
    }
    if (wasEmpty)
      notEmptyCond.notify_all();
  }


  /*! get element that got written into the queue */
  template<typename T>
  void ProducerConsumerQueue<T>::getAll(std::vector<T> &all)
  {
    std::unique_lock<std::mutex> lock(mutex);
    notEmptyCond.wait(lock, [&]{return !content.empty();});

    size_t size = content.size();
    all.resize(size);
    int i = 0;
    for (auto it=content.begin(); it != content.end(); it++)
      all[i++] = *it;
    content.clear();
  }

  /*! get element that got written into the queue */
  template<typename T>
  size_t ProducerConsumerQueue<T>::getSome(T *some, size_t maxSize)
  {
    std::unique_lock<std::mutex> lock(mutex);
    notEmptyCond.wait(lock, [&]{return !content.empty();});

    size_t num = 0;
    while (num < maxSize && !content.empty()) {
      some[num++] = content.front();
      content.pop_front();
    }
    return num;
  }

  template<typename T>
  size_t ProducerConsumerQueue<T>::getSomeFor(T *some, size_t maxSize, std::chrono::milliseconds timeOut)
  {
    using namespace std::chrono;
    std::unique_lock<std::mutex> lock(mutex);
    if (!notEmptyCond.wait_for(lock, timeOut, [&]{return !content.empty();})) {
      return 0;
    }

    size_t num = 0;
    while (num < maxSize && !content.empty()) {
      some[num++] = content.front();
      content.pop_front();
    }
    return num;
  }

} // ::ospray


