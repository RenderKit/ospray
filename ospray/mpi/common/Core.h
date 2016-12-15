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

/*! \file ospray/common/Core.h Defines an abstration that allows
    components to query common properties about the ospray core 

    \detailed Essentially, the "core" is the equivalent to what a
    "device" is to the outside: The device gives _outsiders_ a common
    API to talk to whatever the ray tracer implementation might be,
    and abstracts from, say, whether ospray si running in distributed
    mode, in local mode, etc. The core gives _inside_ components
    something similarly
*/

#include "common/OSPCommon.h"

namespace ospray {
  
  namespace core {

    /*! returns true if and only if the system runs in MPI-parallel mode */
    bool isMpiParallel();

    /*! \brief get number of worker processes that actually do perform rendering work. 

      \detailed In MPI mode, this returns the number of nodes that
      actually perofrm rendering work, EXCLUDING processes (such as
      the master) that only coordinate rendering but don't hold data
      of their own. In localdevice mode, this will return '1'.
     */
    int64 getWorkerCount();

    /*! \brief return rank of the worker

      \detailed In MPI mode, this will return a value that enumerates
      all worker processes from 0 .. getWorkerCount-1. Note that this
      number is NOT to be confused with the MPI rank of the worker.
      On any MPI node that's not a worker, this value will reutrn a
      value < 0.  In local mode, this will return 0.

     */
    index_t getWorkerRank();

  } // ::ospray::core
  
} // ::ospray

  

