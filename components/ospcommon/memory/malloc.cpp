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

#include "malloc.h"
#include "../intrinsics.h"
#if defined(TASKING_TBB)
#  define __TBB_NO_IMPLICIT_LINKAGE 1
#  include "tbb/scalable_allocator.h"
#endif

#ifdef _WIN32
#include <malloc.h>
#endif

namespace ospcommon {
  namespace memory {

    void* alignedMalloc(size_t size, size_t align)
    {
      assert((align & (align-1)) == 0);
#if defined(TASKING_TBB)
      return scalable_aligned_malloc(size,align);
#else
#  ifdef _WIN32
      return _aligned_malloc(size, align);
#  else // __UNIX__
      return _mm_malloc(size, align);
#  endif
#endif
    }

  void alignedFree(void* ptr)
  {
#if defined(TASKING_TBB)
      scalable_aligned_free(ptr);
#else
#  ifdef _WIN32
      return _aligned_free(ptr);
#  else // __UNIX__
      _mm_free(ptr);
#  endif
#endif
    }

  } // ::ospcommon::memory
} // ::ospcommon
