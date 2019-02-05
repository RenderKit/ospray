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

#include "../common.h"

namespace ospcommon {
 namespace utility {

  template <typename T>
  struct DataView
  {
    DataView()  = default;
    ~DataView() = default;

    DataView(const void *data, size_t stride = sizeof(T));

    void reset(const void *data, size_t stride = sizeof(T));
    const T& operator[](size_t index) const;

  protected:

    const byte_t* ptr{nullptr};
    size_t stride{1};
  };

  // Inlined member definitions ///////////////////////////////////////////////

  template <typename T>
  inline DataView<T>::DataView(const void *_data, size_t _stride)
    : ptr(static_cast<const byte_t*>(_data)), stride(_stride)
  { }

  template <typename T>
  inline void DataView<T>::reset(const void *_data, size_t _stride)
  {
    ptr    = static_cast<const byte_t*>(_data);
    stride = _stride;
  }

  template <typename T>
  inline const T& DataView<T>::operator[](size_t index) const
  {
    return *reinterpret_cast<const T*>(ptr + (index * stride));
  }

 } // ::ospcommon::utility
} // ::ospcommon
