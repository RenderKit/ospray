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

#pragma once

#include "Messaging.h"
#include <map>
/*! \file ospray/mpi/async/CommLayer.h Extends the asynchronous
  messaging protocal (that can send messages asynchronously from one
  rank to another) by a communication layer that allows sending from
  one *object* in a rank to another *object* in another rank using a
  global registry of distributed objects */

namespace ospray {
  namespace mpi {
    namespace async {
      
      /*! object that handles the registration of distributed
        objects. objects can send messages to other objects (on other
        nodes) that are registered on thsi comm layer (to be more
        exact: to objects registered at the _remote_ instance of the
        commlayer - though a distributed objects would usually
        register itself on each rank's instance of the commlayer, in
        principle it could be registered only one one (say, the
        master), and others can send to that as long as they know the
        ID at the rank it is registered at).

        The commlayer object itself is a async::Consumer - it receives
        messages from other nodes, and then routes them to the
        respective object(s).
      */
      struct CommLayer : public async::Consumer { 
        typedef int32 ObjectID; 

        struct Address {
          Address(int32 rank=-1,ObjectID objectID=-1) 
            : rank(rank), objectID(objectID)
          {}

          int32    rank;
          ObjectID objectID;
        };

        /*! a common header used for all messages sent over the
          commlayer, to enable proper routing at the remote node. */
        struct Message {
          /*! @{ internal header - do not use from the
            application. Note we have to have this field inside the
            messsage, because the messaging protocal doesn't send
            headers and messages separately, but requires both to be
            a single block of data */
          Address source; /*!< address of sender that sent this
                             message. commlayer will fill this in,
                             user doesn't have to carea */
          Address dest; /*!< address of sender that will receive this
                          message. commlayer will fill this in,
                          user doesn't have to carea */
          size_t  size; /*!< size, in bytes. Commlayer will fill that
                           in, user doesn't have to care. */

          int64   command; /*!< optional command field similar to an
                           MPI tag (allows to send messages similar
                           to some RPC protocol). may be unused */
          /*! @} */
        };

        /*! abstraction for an object that can send and receive messages */
        struct Object {
          Object(CommLayer *comm, ObjectID myID)
            : myID(myID), comm(comm)
          {}

          /*! function that will be called by the commlayer as soon as
              a message for this object comes in. this function should
              execute as soon as possible - needing a seprate thread
              to actually do heavy work if needed - because this
              function may actually block the system's receiver thread
              until complete */
          virtual void incoming(Message *msg) = 0;

          ObjectID myID;
          CommLayer *comm;
        };

        void sendTo(Address dest, Message *msg, size_t size);

        //! the async::consumer virtual callback that a async::message arrived
        virtual void process(const Address &source, void *message, int32 size);

        void registerObject(Object *object, ObjectID ID);

        //! convenience function that returns our rank in the comm group 
        int32 rank() const { return group->rank; }

        //! mutex to protect the registry
        Mutex mutex;

        /*! registry that allows to lookup objects by handle. objects
            have to register themselves in order to be reachable */
        std::map<ObjectID,Object *> registry;

        /*! the mpi::async group we're using to communicate with
            remote nodes */
        async::Group *group; 
      };

    } // ::ospray::mpi::async
  } // ::ospray::mpi
} // ::ospray
