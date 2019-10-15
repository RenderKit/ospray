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

#include "ManagedObject.h"
#include "Traits.h"
// stl
#include <array>
#include <vector>

namespace ospray {
  namespace cpp {

    class Data : public ManagedObject<OSPData, OSP_DATA>
    {
     public:
      // Generic construction from an existing array

      Data(size_t numItems,
           OSPDataType format,
           const void *init = nullptr,
           bool isShared    = false);

      // Adapters to work with existing STL containers

      template <typename T, std::size_t N>
      Data(const std::array<T, N> &arr, bool isShared = false);

      template <typename T, typename ALLOC_T>
      Data(const std::vector<T, ALLOC_T> &arr, bool isShared = false);

      // Set a single object as a 1-item data array of handles

      template <typename T, OSPDataType TY>
      Data(ManagedObject<T, TY> obj);

      // Construct from an existing OSPData handle (Data then owns lifetime)

      Data(OSPData existing = nullptr);
    };

    static_assert(sizeof(Data) == sizeof(OSPData),
                  "cpp::Data can't have data members!");

    // Inlined function definitions ///////////////////////////////////////////

    inline Data::Data(size_t numItems,
                      OSPDataType format,
                      const void *init,
                      bool isShared)
    {
      if (isShared) {
        ospObject = ospNewSharedData(init, format, numItems);
      } else {
        ospObject = ospNewData(format, numItems);
        auto tmp  = ospNewSharedData(init, format, numItems);
        ospCopyData(tmp, ospObject);
        ospRelease(tmp);
      }
    }

    template <typename T, std::size_t N>
    inline Data::Data(const std::array<T, N> &arr, bool isShared)
        : Data(N, OSPTypeFor<T>::value, arr.data(), isShared)
    {
      static_assert(
          OSPTypeFor<T>::value != OSP_UNKNOWN,
          "Only types corresponding to OSPDataType values can be set "
          "as elements in OSPRay Data arrays. NOTE: Math types (vec, "
          "box, linear, affine) are expected to come from ospcommon::math.");
    }

    template <typename T, typename ALLOC_T>
    inline Data::Data(const std::vector<T, ALLOC_T> &arr, bool isShared)
        : Data(arr.size(), OSPTypeFor<T>::value, arr.data(), isShared)
    {
      static_assert(
          OSPTypeFor<T>::value != OSP_UNKNOWN,
          "Only types corresponding to OSPDataType values can be set "
          "as elements in OSPRay Data arrays. NOTE: Math types (vec, "
          "box, linear, affine) are expected to come from ospcommon::math.");
    }

    template <typename T, OSPDataType TY>
    inline Data::Data(ManagedObject<T, TY> obj) : Data(1, TY, &obj)
    {
    }

    inline Data::Data(OSPData existing)
        : ManagedObject<OSPData, OSP_DATA>(existing)
    {
    }

  }  // namespace cpp

  OSPTYPEFOR_SPECIALIZATION(cpp::Data, OSP_DATA);

}  // namespace ospray
