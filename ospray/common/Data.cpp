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

// ospray
#include "Data.h"
#include "ospray/ospray.h"

namespace ospray {

  Data::Data(size_t numItems, OSPDataType type, const void *init, int flags) :
    numItems(numItems),
    numBytes(numItems * sizeOf(type)),
    flags(flags),
    type(type)
  {
    if (flags & OSP_DATA_SHARED_BUFFER) {
      Assert2(init != NULL, "shared buffer is NULL");
      data = const_cast<void *>(init);
    } else {
      data = alignedMalloc(numBytes+16);
      if (init)
        memcpy(data,init,numBytes);
      else if (type == OSP_OBJECT)
        memset(data,0,numBytes);
    }

    managedObjectType = OSP_DATA;
  }

  Data::~Data()
  {
    if (type == OSP_OBJECT) {
      Data **child = (Data **)data;
      for (uint32_t i = 0; i < numItems; i++) {
        if (child[i])
          child[i]->refDec();
      }
    }

    if (!(flags & OSP_DATA_SHARED_BUFFER))
      alignedFree(data);
  }

  /*! commit this object - for this object type, make sure that all
    listeners that have registered know that we have changed */
  void Data::commit()
  {
    notifyListenersThatObjectGotChanged();
  }

  std::string Data::toString() const
  {
    return "ospray::Data";
  }

  size_t ospray::Data::size() const
  {
    return numItems;
  }

} // ::ospray
