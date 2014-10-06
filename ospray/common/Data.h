/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// ospray stuff
#include "Managed.h"

namespace ospray {

  /*! \brief defines a data array (aka "buffer") type that contains
      'n' items of a given type */
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
