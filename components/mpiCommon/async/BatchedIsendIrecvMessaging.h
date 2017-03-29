// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

// ours
#include "Messaging.h"
// ospcommon
#include "ospcommon/ProducerConsumerQueue.h"
// stl
#include <deque>
#include <vector>
#include <thread>
#include <atomic>

namespace ospray {
  namespace mpi {
    namespace async {
      struct BatchedIsendIrecvImpl : public AsyncMessagingImpl
      {
        struct Group;
        struct Message;

        struct ThreadBase
        {
          std::thread handle;
        };
        
        /*! message _sender_ thread */
        struct SendThread : public ThreadBase {
          SendThread(Group *group) : group(group) {}
          virtual void run();

          Group *group;
        };

        /*! message _probing_ thread - continuously probes for newly
            incoming messages, and puts them into recv queue */
        struct RecvThread : public ThreadBase
        {
          RecvThread(Group *group) : group(group) {}

          virtual void run();
          Group *group;
        };

        /*! message _processing_ thread */
        struct ProcThread : public ThreadBase
        {
          ProcThread(Group *group) : group(group)
          {
#ifdef OSPRAY_PIN_ASYNC
            embree::setAffinity(57); // 56
#endif
          }

          virtual void run();

          Group *group;
        };

        /*! an 'action' (either a send or a receive, or a
            processmessage) to be performed by a thread; what action
            it is depends on the queue it is in */
        struct Action
        {
          void   *data;
          int32   size;
          Address addr;
          MPI_Request request;
        };

        struct Group : public mpi::async::Group
        {
          Group(MPI_Comm comm, Consumer *consumer, int32 tag = MPI_ANY_TAG);
          void shutdown();
          void flush();
          
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

          std::mutex flushMutex;
          std::condition_variable isFlushedCondition;
          size_t numMessagesDoneSending { 0 };
          size_t numMessagesAskedToSend { 0 };
        };

        virtual void flush() override;
        virtual void init() override;
        virtual void shutdown() override;
        virtual async::Group *createGroup(MPI_Comm comm,
                                          Consumer *consumer,
                                          int32 tag = MPI_ANY_TAG) override;
        virtual void send(const Address &dest,
                          void *msgPtr,
                          int32 msgSize) override;

        std::vector<Group *> myGroups;
      };

    }
  }
}
