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

#pragma once

#include "../common.h"

namespace ospcommon {
 namespace utility {

  // simple version of ArrayView, but with stride
  ///////////////////////////////////////////////

  template <typename T>
  struct DataView
  {
    DataView() = delete;
    ~DataView() = default;

    explicit DataView(T *data, size_t stride = 1);

    void reset(T *data, size_t stride = 1);
    T& operator[](size_t index) const;

  private:
    T*     ptr{nullptr};
    size_t stride{1};
  };

  template <typename T>
  inline DataView<T>::DataView(T *_data, size_t _stride)
    : ptr(_data), stride(_stride)
  { }

  template <typename T>
  inline void DataView<T>::reset(T *_data, size_t _stride)
  {
    ptr    = _data;
    stride = _stride;
  }

  template <typename T>
  inline T& DataView<T>::operator[](size_t index) const
  {
    return ptr[index*stride];
  }

 } // ::ospcommon::utility
} // ::ospcommon
