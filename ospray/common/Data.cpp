// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// ospray
#include "Data.h"
#include "ospray/ospray.h"
// stl
#include <sstream>

namespace ospray {

  Data::Data(size_t numItems, OSPDataType type, void *init, int flags) :
    numItems(numItems),
    numBytes(numItems * sizeOf(type)),
    type(type),
    flags(flags)
  {
    /* two notes here:
       a) i'm using embree's 'new' to enforce alignment
       b) i'm adding 16 bytes to size to enforce 4-float padding (which embree
          requires in some buffers
    */
    if (flags & OSP_DATA_SHARED_BUFFER) {
      Assert2(init != NULL, "shared buffer is NULL");
      data = init;
    } else {
      data = alignedMalloc(numBytes+16);
      if (init)
        memcpy(data,init,numBytes);
      else if (type == OSP_OBJECT)
        memset(data,0,numBytes);
    }

    managedObjectType = OSP_DATA;

    // std::cout << "checksum when creating data array" << std::endl;
    // PRINT(numBytes);
    // PRINT((int*)computeCheckSum(init,numBytes));
  }

  Data::~Data()
  {
    if (type == OSP_OBJECT) {
      Data **child = (Data **)data;
      for (int i=0;i<numItems;i++) if (child[i]) child[i]->refDec();
    }
    if (!(flags & OSP_DATA_SHARED_BUFFER)) alignedFree(data);
  }

} // ::ospray
