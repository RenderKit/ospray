// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "OSPCommon.h"
// stl
#include <queue>

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

    /*! get element that got written into the queue */
    T get();

  private:
    /*! the actual queue that holds the data */
    std::deque<T> content;
    /*! mutex to allow thread-safe access */
    Mutex mutex;
    /*! condition that is triggered when queue is not empty. 'get'
        requests can wait on this */
    Condition notEmptyCond;
  };


  // =======================================================
  // IMPLEMENTATION SECTION
  // =======================================================


    /*! get element that got written into the queue */
  template<typename T>
  T ProducerConsumerQueue<T>::get()
  {
    mutex.lock();
    while (content.empty())
      notEmptyCond.wait(mutex);
    bool wasEmpty = content.empty();
    T t = content.front();
    content.pop_front();
    mutex.unlock();
    return t;
  }
  
  /*! put a new element into this queue */
  template<typename T>
  void ProducerConsumerQueue<T>::put(T t) 
  {
    mutex.lock();
    bool wasEmpty = content.empty();
    content.push_back(t);
    if (wasEmpty)
      notEmptyCond.broadcast();
    mutex.unlock();
  }

}

