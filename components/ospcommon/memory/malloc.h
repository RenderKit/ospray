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
  namespace memory {

#define ALIGN_PTR(ptr,alignment) \
  ((((size_t)ptr)+alignment-1)&((size_t)-(ssize_t)alignment))

      /*! aligned allocation */
      OSPCOMMON_INTERFACE void* alignedMalloc(size_t size, size_t align = 64);
      OSPCOMMON_INTERFACE void alignedFree(void* ptr);

      template <typename T>
       __forceinline T* alignedMalloc(size_t nElements, size_t align = 64)
      {
        return (T*)alignedMalloc(nElements*sizeof(T), align);
      }

      inline bool isAligned(void *ptr, int alignment = 64)
      {
        return reinterpret_cast<size_t>(ptr) % alignment == 0;
      }

    // NOTE(jda) - can't use function wrapped alloca solution as Clang won't
    //             inline  a function containing alloca()...but works w/ gcc+icc
#if 0
    template<typename T>
    __forceinline T* stackBuffer(size_t nElements)
    {
      return static_cast<T*>(alloca(sizeof(T) * nElements));
    }
#else
#  define STACK_BUFFER(TYPE, nElements) (TYPE*)alloca(sizeof(TYPE)*nElements)
#endif

  } // ::ospcommon::memory
} // ::ospcommon

