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

#include "CommLayer.h"

/*! \file ospray/mpi/async/CommLayer.h Extends the asynchronous
  messaging protocal (that can send messages asynchronously from one
  rank to another) by a communication layer that allows sending from
  one *object* in a rank to another *object* in another rank using a
  global registry of distributed objects */

namespace ospray {
  namespace mpi {
    namespace async {

      CommLayer *CommLayer::WORLD = nullptr;

      CommLayer::Object::Object(CommLayer *comm, ObjectID myID)
        : myID(myID), comm(comm)
      {
        master = mpi::async::CommLayer::Address(comm->masterRank(),myID);
        worker = new mpi::async::CommLayer::Address[comm->numWorkers()];
        for (int i = 0 ; i < comm->numWorkers(); i++)
          worker[i] = mpi::async::CommLayer::Address(comm->workerRank(i),myID);
      }

      void CommLayer::process(const mpi::Address &source,
                              void *message,
                              int32 size)
      {
        UNUSED(size);
        Message *msg = (Message*)message;

        Object *obj = nullptr;
        {
          SCOPED_LOCK(mutex);
          obj = registry[msg->dest.objectID];
        }

        if (!obj) {
          throw std::runtime_error("#osp:mpi:CommLayer: no object with given "
                                   "ID");
        }

        msg->source.rank  = source.rank;
        obj->incoming(msg);
      }

      void CommLayer::sendTo(Address dest, Message *msg, size_t size)
      {
        msg->source.rank = group->rank;
        msg->dest = dest;
        msg->size = size;
        async::send(mpi::Address(group,dest.rank),msg,size);
      }

      void CommLayer::registerObject(Object *object, ObjectID ID)
      {
        /* WARNING: though we do protect the registry here, the
           registry lookup itself is NOT thread-safe right now */
        SCOPED_LOCK(mutex);
        assert(registry.find(ID) == registry.end());
        registry[ID] = object;
      }

    } // ::ospray::mpi::async
  } // ::ospray::mpi
} // ::ospray
