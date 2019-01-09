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

#include <array>
#include <vector>

namespace ospcommon {
  namespace utility {

    /*  'ArrayView<T>' implements an array interface on a pointer to data which
     *  is *NOT* owned by ArrayView. If you want ArrayView to own data, then
     *  instead use std::array<T> or std::vector<T>.
     */
    template <typename T>
    struct ArrayView
    {
      ArrayView() = default;
      ~ArrayView() = default;

      template <size_t SIZE>
      ArrayView(std::array<T, SIZE> &init);

      ArrayView(std::vector<T> &init);

      explicit ArrayView(T *data, size_t size);

      void reset();
      void reset(T *data, size_t size);

      template <size_t SIZE>
      ArrayView& operator=(std::array<T, SIZE> &rhs);

      ArrayView& operator=(std::vector<T> &rhs);

      size_t size() const;

      T& operator[](size_t offset) const;
      T& at(size_t offset) const;// bounds checking

      operator bool() const;
      operator T*() const;

      T* data() const;

      T* begin() const;
      T* end()   const;

      const T* cbegin() const;
      const T* cend()   const;

    private:

      T*     ptr{nullptr};
      size_t numItems{0};
    };

    // Inlined ArrayView definitions //////////////////////////////////////////

    template <typename T>
    inline ArrayView<T>::ArrayView(T *_data, size_t _size)
      : ptr(_data), numItems(_size)
    {
    }

    template <typename T>
    template <size_t SIZE>
    inline ArrayView<T>::ArrayView(std::array<T, SIZE> &init)
      : ptr(init.data()), numItems(init.size())
    {
    }

    template <typename T>
    inline ArrayView<T>::ArrayView(std::vector<T> &init)
      : ptr(init.data()), numItems(init.size())
    {
    }

    template <typename T>
    inline void ArrayView<T>::reset()
    {
      ptr      = nullptr;
      numItems = 0;
    }

    template <typename T>
    inline void ArrayView<T>::reset(T *_data, size_t _size)
    {
      ptr      = _data;
      numItems = _size;
    }

    template <typename T>
    template <size_t SIZE>
    inline ArrayView<T>&
    ArrayView<T>::operator=(std::array<T, SIZE> &rhs)
    {
      ptr      = rhs.data();
      numItems = rhs.size();

      return *this;
    }

    template <typename T>
    inline ArrayView<T>& ArrayView<T>::operator=(std::vector<T> &rhs)
    {
      ptr      = rhs.data();
      numItems = rhs.size();

      return *this;
    }

    template <typename T>
    size_t ArrayView<T>::size() const
    {
      return numItems;
    }

    template <typename T>
    inline T& ArrayView<T>::operator[](size_t offset) const
    {
      return ptr[offset];
    }

    template <typename T>
    inline T& ArrayView<T>::at(size_t offset) const
    {
      if (offset >= size())
        throw std::runtime_error("ArrayView<T>: out of bounds access!");

      return ptr[offset];
    }

    template <typename T>
    inline ArrayView<T>::operator bool() const
    {
      return ptr != nullptr;
    }

    template <typename T>
    inline ArrayView<T>::operator T*() const
    {
      return ptr;
    }

    template <typename T>
    T* ArrayView<T>::data() const
    {
      return ptr;
    }

    template <typename T>
    T* ArrayView<T>::begin() const
    {
      return ptr;
    }

    template <typename T>
    T* ArrayView<T>::end() const
    {
      return ptr + size();
    }

    template <typename T>
    const T* ArrayView<T>::cbegin() const
    {
      return ptr;
    }

    template <typename T>
    const T* ArrayView<T>::cend() const
    {
      return ptr + size();
    }

    // ArrayView utility functions ////////////////////////////////////////////

    template <typename T>
    inline ArrayView<T> make_ArrayView(T *data, size_t size)
    {
      return ArrayView<T>(data, size);
    }

  } // ::ospcommon::utility
} // ::ospcommon
