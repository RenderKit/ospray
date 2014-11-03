/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "sg/common/Common.h"

namespace ospray {
  namespace sg {
    
    //! \brief Implements an abstraction of Time
    /*! Abstracts the concept of time to be used for time-stamping
      node's last 'lastupdated' and /lastmodified' time stamps */
    struct TimeStamp {
      //! \brief constructor 
      TimeStamp(uint64 t) : t(t) {};
      
      //! \brief returns global time(stamp) at time of calling
      static inline TimeStamp now() { return __rdtsc(); }

      //! \brief Allows ot typecast to a uint64 (so times can be compared)
      inline operator uint64 () const { return t; }
      
    private:
      //! \brief the uint64 that stores the time value
      uint64 t;
    };

  } // ::ospray::sg
} // ::ospray


