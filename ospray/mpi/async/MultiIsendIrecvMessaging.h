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
#include <deque>

namespace ospray {
  namespace mpi {
    namespace async {
      struct MultiIsendIrecvImpl : public AsyncMessagingImpl {
        struct QueuedMessage {
          void   *ptr;
          int32   size;
          Address addr;
          MPI_Request request;
          MPI_Status  status;
        };

        struct Group : public mpi::async::Group {

          Group(const std::string &name, MPI_Comm comm, 
                Consumer *consumer, int32 tag = MPI_ANY_TAG);
          void shutdown();

          static void sendThreadFunc(void *arg);
          static void recvThreadFunc(void *arg);

          thread_t    sendThread;
          thread_t    recvThread;
          Mutex       mutex;
          Condition   cond;
          MPI_Request currentSendRequest;
          
          std::vector<QueuedMessage*> sendQueue;
          bool      beginShutDown, sendIsShutDown, recvIsShutDown;
        };

        virtual void init();
        virtual void shutdown();
        virtual async::Group *createGroup(const std::string &name, 
                                        MPI_Comm comm, 
                                        Consumer *consumer, 
                                        int32 tag = MPI_ANY_TAG);
        virtual void send(const Address &dest, void *msgPtr, int32 msgSize);

        std::vector<Group *> myGroups;
      };
    };
  }
}
