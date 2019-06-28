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

    /*! Iterator begin/end for Data arrays */
    template <typename T>
    T *begin();

    template <typename T>
    T *end();

    template <typename T>
    const T *begin() const;

    template <typename T>
    const T *end() const;

    template <typename T>
    T &at(size_t i);

    template <typename T>
    const T &at(size_t i) const;

    // Data members //

    void *data;       /*!< pointer to data */
    size_t numItems;  /*!< number of items */
    size_t numBytes;  /*!< total num bytes (sizeof(type)*numItems) */
    int flags;        /*!< creation flags */
    OSPDataType type; /*!< element type */
  };

  // Inlined definitions //////////////////////////////////////////////////////

  template <typename T>
  inline T *Data::begin()
  {
    return static_cast<T *>(data);
  }

  template <typename T>
  inline T *Data::end()
  {
    return begin<T>() + numItems;
  }

  template <typename T>
  inline const T *Data::begin() const
  {
    return static_cast<const T *>(data);
  }

  template <typename T>
  inline const T *Data::end() const
  {
    return begin<const T>() + numItems;
  }

  template <typename T>
  inline T &Data::at(size_t i)
  {
    return *(begin<T>() + i);
  }

  template <typename T>
  inline const T &Data::at(size_t i) const
  {
    return *(begin<T>() + i);
  }

  // Helper functions /////////////////////////////////////////////////////////

  inline std::vector<void *> createArrayOfIE(Data &data)
  {
    if (data.type != OSP_OBJECT)
      throw std::runtime_error("cannot createArrayOfIE() with non OSP_OBJECT!");

    std::vector<void *> retval;
    retval.resize(data.size());

    auto begin = data.begin<ManagedObject *>();
    auto end   = data.end<ManagedObject *>();
    std::transform(begin, end, retval.begin(), [](ManagedObject *obj) {
      return obj->getIE();
    });

    return retval;
  }

}  // namespace ospray
