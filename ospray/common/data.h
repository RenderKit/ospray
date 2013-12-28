#pragma once

// ospray stuff
#include "managed.h"

namespace ospray {

  /*! defines a data array (aka "buffer") type that contains 'n' items of a given type */
  struct Data : public ManagedObject
  {
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

}
