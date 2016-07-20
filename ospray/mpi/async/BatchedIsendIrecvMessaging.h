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

// ospray
#include "Messaging.h"
#include "common/Thread.h"
#include "common/ProducerConsumerQueue.h"
// stl
#include <deque>
#include <vector>

namespace ospray {
  namespace mpi {
    namespace async {
      struct BatchedIsendIrecvImpl : public AsyncMessagingImpl {
        
        struct Group;
        struct Message;

        /*! message _sender_ thread */
        struct SendThread : public Thread {
          SendThread(Group *group) : group(group) {
#ifdef OSPRAY_PIN_ASYNC
            embree::setAffinity(58); // 58
#endif
          };
          virtual void run();

          Group *group;
        };
        /*! message _probing_ thread - continuously probes for newly
            incoming messages, and puts them into recv queue */
        struct RecvThread : public Thread {
          RecvThread(Group *group) : group(group) {
#ifdef OSPRAY_PIN_ASYNC
            embree::setAffinity(55); // 55
#endif
          };

          virtual void run();
          Group *group;
        };
        /*! message _processing_ thread */
        struct ProcThread : public Thread {
          ProcThread(Group *group) : group(group) {
#ifdef OSPRAY_PIN_ASYNC
            embree::setAffinity(57); // 56
#endif
          };
          virtual void run();

          Group *group;
        };

        /*! an 'action' (either a send or a receive, or a
            processmessage) to be performed by a thread; what action
            it is depends on the queue it is in */
        struct Action {
          void   *data;
          int32   size;
          Address addr;
          MPI_Request request;
        };

        struct Group : public mpi::async::Group {

          Group(const std::string &name, MPI_Comm comm, 
                Consumer *consumer, int32 tag = MPI_ANY_TAG);
          void shutdown();

          /*! the queue new send requests are put into; the send
              thread pulls from this and sends ad infinitum */

          ProducerConsumerQueue<Action *> sendQueue;
          /*! the queue that newly received messages are put in; the
              reiver thread puts new messages in here, the processing
              thread pulls from here and processes */
          ProducerConsumerQueue<Action *> recvQueue;

          SendThread sendThread;
          ProcThread procThread;
          RecvThread recvThread;
          std::atomic<bool> shouldExit;
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
