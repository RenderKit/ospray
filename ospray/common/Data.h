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

#pragma once

// ospray
#include "Managed.h"

namespace ospray {

  /*! \brief defines a data array (aka "buffer") type that contains
      'n' items of a given type */
  struct OSPRAY_SDK_INTERFACE Data : public ManagedObject
  {
    virtual void commit() { notifyListenersThatObjectGotChanged(); }

    virtual std::string toString() const { return "ospray::Data"; }

    Data(size_t numItems, OSPDataType type, void *data, int flags);
    virtual ~Data();

    /*! return number of items in this data buffer */
    inline size_t size() const { return numItems; }

    void       *data;     /*!< pointer to data */
    size_t      numItems; /*!< number of items */
    size_t      numBytes; /*!< total num bytes (sizeof(type)*numItems) */
    int         flags;    /*!< creation flags */
    OSPDataType type;     /*!< element type */
  };

} // ::ospray
