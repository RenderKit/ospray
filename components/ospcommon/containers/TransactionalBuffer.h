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

#include <mutex>
#include <utility>
#include <vector>

namespace ospcommon {

  template <typename T>
  struct TransactionalBuffer
  {
    TransactionalBuffer() = default;

    // Insert into the buffer (producer)
    void push_back(const T &);
    void push_back(T &&);

    // Take all contents of the buffer (consumer)
    std::vector<T> consume();

    size_t size() const;

    bool empty() const;

  private:

    // Data members //

    std::vector<T> buffer;

    mutable std::mutex bufferMutex;//NOTE(jda) - Marked mutable so 'const'
                                   //            methods can take the lock...
  };

  // Inlined members //////////////////////////////////////////////////////////

  template<typename T>
  inline void TransactionalBuffer<T>::push_back(const T &v)
  {
    std::lock_guard<std::mutex> lock(bufferMutex);
    buffer.push_back(v);
  }

  template<typename T>
  inline void TransactionalBuffer<T>::push_back(T &&v)
  {
    std::lock_guard<std::mutex> lock(bufferMutex);
    buffer.push_back(std::forward<T>(v));
  }

  template<typename T>
  inline std::vector<T> TransactionalBuffer<T>::consume()
  {
    std::lock_guard<std::mutex> lock(bufferMutex);
    return std::move(buffer);
  }

  template<typename T>
  inline size_t TransactionalBuffer<T>::size() const
  {
    std::lock_guard<std::mutex> lock(bufferMutex);
    return buffer.size();
  }

  template<typename T>
  inline bool TransactionalBuffer<T>::empty() const
  {
    std::lock_guard<std::mutex> lock(bufferMutex);
    return buffer.empty();
  }

} // ::ospcommon
