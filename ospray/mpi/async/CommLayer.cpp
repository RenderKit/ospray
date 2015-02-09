// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include "CommLayer.h"

/*! \file ospray/mpi/async/CommLayer.h Extends the asynchronous
  messaging protocal (that can send messages asynchronously from one
  rank to another) by a communication layer that allows sending from
  one *object* in a rank to another *object* in another rank using a
  global registry of distributed objects */

namespace ospray {
  namespace mpi {
    namespace async {
      
      void CommLayer::process(const mpi::Address &source, void *message, int32 size)
      {
        NOTIMPLEMENTED;
      }

      void CommLayer::sendTo(Address dest, Message *msg, size_t size)
      {
        NOTIMPLEMENTED;
      }

      void CommLayer::registerObject(Object *object, ObjectID ID)
      {
        /* WARNING: though we do protect the registry here, the
           registry lookup itself is NOT thread-safe right now */
        mutex.lock();
        assert(registry.find(ID) == registry.end());
        registry[ID] = object;
        mutex.unlock();
      }

    } // ::ospray::mpi::async
  } // ::ospray::mpi
} // ::ospray
