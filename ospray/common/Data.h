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

#include "fb/ToneMapperPixelOp.h"

// ospray
#include "Managed.h"

namespace ospray {

  /*! \brief defines a data array (aka "buffer") type that contains
      'n' items of a given type */
  struct OSPRAY_SDK_INTERFACE Data : public ManagedObject
  {
    Data(size_t numItems, OSPDataType type, const void *data, int flags = 0);

    virtual ~Data() override;

    /*! commit this object - for this object type, make sure that all
        listeners that have registered know that we have changed */
    virtual void commit() override;

    /*! pretty-print this object, for debugging purposes */
    virtual std::string toString() const override;

    /*! return number of items in this data buffer */
    size_t size() const;

    /*! run the passed function over each element of the buffer in a for loop */
    template<typename T, typename Fn>
    void forEach(Fn fn)
    {
      // TODO WILL: Kind of assumes that the types stored match with the
      // type T the caller requested
      const size_t typeSize = sizeOf(type);
      uint8_t *buf = static_cast<uint8_t*>(data);
      for (size_t i = 0; i < numItems; ++i) {
        // Note: need double ptr if T is an object type
        T **val = reinterpret_cast<T**>(buf + i * typeSize); 
        fn(*val);
      }
    }

    // Data members //

    void       *data;     /*!< pointer to data */
    size_t      numItems; /*!< number of items */
    size_t      numBytes; /*!< total num bytes (sizeof(type)*numItems) */
    int         flags;    /*!< creation flags */
    OSPDataType type;     /*!< element type */
  };

} // ::ospray
