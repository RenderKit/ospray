
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

#include "MultiIsendIrecvMessaging.h"

namespace ospray {
  namespace mpi {
    namespace async {
      int dbg = false;

      MultiIsendIrecvImpl::Group::Group(const std::string &name, MPI_Comm comm, 
                                       Consumer *consumer, int32 tag)
        : async::Group(// name,
                       comm,consumer,tag),
          sendThreadState(NOT_STARTED), recvThreadState(NOT_STARTED)
      {
        recvThread = embree::createThread(recvThreadFunc,this);
        sendThread = embree::createThread(sendThreadFunc,this);
      }

      void MultiIsendIrecvImpl::Group::sendThreadFunc(void *arg)
      {
        Group *g = (Group *)arg;
        g->sendThreadState = RUNNING;
        std::vector<QueuedMessage *> activeSendQueue;

        while (1) {
          g->sendMutex.lock();
          if (dbg) {
            printf("%i: TESTING WAIT...\n",world.rank); fflush(0);
          }
          while (activeSendQueue.empty() && 
                 g->sendQueue.empty() && 
                 !(g->sendThreadState == FLAGGED_TO_TERMINATE)) 
            {
              if (dbg) {
                printf("%i: waiting again...\n",world.rank); fflush(0);
              }
              g->sendCond.wait(g->sendMutex);
            }
          if (dbg) {
            printf("%i: new-sendq-size %li, active-sendq-size %li\n",
                   world.rank,
                   g->sendQueue.size(),
                   activeSendQueue.size());
          }

          if (g->sendThreadState == FLAGGED_TO_TERMINATE) {
            printf("EXITING %i/%i\n",world.rank,world.size);fflush(0);
            break;
          }
          
          // ------------------------------------------------------------------
          // pull newly added send messages from queue, then release that queue
          // ------------------------------------------------------------------
          std::vector<QueuedMessage *> newSendQueue = g->sendQueue;
          g->sendQueue.clear();
          g->sendMutex.unlock();
          
          // ------------------------------------------------------------------
          // start new messages, and add to active send queue
          // ------------------------------------------------------------------
          for (int i=0;i<newSendQueue.size();i++) {
            QueuedMessage *msg = newSendQueue[i];
            if (msg->size == 4) {
              printf("rank %i found new 4-byte message to %i\n",world.rank,msg->addr.rank);
              fflush(0);
            }
            MPI_CALL(Isend(msg->ptr,msg->size,MPI_BYTE,msg->addr.rank,g->tag,
                           msg->addr.group->comm,&msg->request));
            activeSendQueue.push_back(msg);
          }

          // ------------------------------------------------------------------
          // remove all messages that are done sending form active send queue
          // ------------------------------------------------------------------
          for (int i=0;i<activeSendQueue.size();) {
            QueuedMessage *msg = activeSendQueue[i];
            int done = 0;
            MPI_CALL(Test(&msg->request,&done,&msg->status));
            if (done) {
              MPI_CALL(Wait(&msg->request,&msg->status));
              activeSendQueue.erase(activeSendQueue.begin()+i);
              free(msg->ptr);
              delete msg;
            } else {
              ++i;
            }
          }

          // if (activeSendQueue.empty()) usleep(10);
        }

        if (!activeSendQueue.empty() || !g->sendQueue.empty()) {
          printf("#osp:mpi(%2i): flushing send queue\n",mpi::world.rank);
          std::vector<QueuedMessage *> newSendQueue = g->sendQueue;
          g->sendQueue.clear();
          for (int i=0;i<newSendQueue.size();i++) {
            QueuedMessage *msg = newSendQueue[i];
            if (msg->size == 4) {
              printf("rank %i found new 4-byte message to %i\n",world.rank,msg->addr.rank);
              fflush(0);
            }
            MPI_CALL(Isend(msg->ptr,msg->size,MPI_BYTE,msg->addr.rank,g->tag,
                           msg->addr.group->comm,&msg->request));
            activeSendQueue.push_back(msg);
          }

          for (int i=0;i<activeSendQueue.size();i++) {
            QueuedMessage *msg = activeSendQueue[i];
            MPI_CALL(Wait(&msg->request,&msg->status));
            // MPI_CALL(Cancel(&msg->request));
          }
        }
        // assert(activeSendQueue.empty());
        printf("#osp:mpi(%2i): shutting down send thread\n",g->rank);fflush(0);
        g->sendThreadState = TERMINATED;
        g->sendTerminatedCond.broadcast();
        g->sendMutex.unlock();
      }

      void MultiIsendIrecvImpl::Group::recvThreadFunc(void *arg)
      {
        Group *g = (Group *)arg;
        g->recvThreadState = RUNNING;

        while (1) {
          MPI_Status status;
          MPI_CALL(Probe(MPI_ANY_SOURCE,g->tag,g->comm,&status));
          // printf("#osp:mpi(%2i): incoming from %i\n",g->rank,status.MPI_SOURCE);fflush(0);
          if (g->recvThreadState == FLAGGED_TO_TERMINATE) {
            printf("#osp:mpi(%2i): recv flagged to terminate\n",g->rank);fflush(0);
            int done;
            MPI_CALL(Recv(&done,1,MPI_INT,status.MPI_SOURCE,status.MPI_TAG,g->comm,&status));
            break;
          }
          
          int count = -1;
          MPI_CALL(Get_count(&status,MPI_BYTE,&count));
          static int lastCount = -1;
          if (count != lastCount)  {
            printf("rank %i received %i bytes\n",world.rank,count);
            fflush(0);
            lastCount = count;
          }
          void *msg = malloc(count);
          MPI_CALL(Recv(msg,count,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,
                        g->comm,&status));
          g->consumer->process(Address(g,status.MPI_SOURCE),msg,count);
        }
        // g->recvMutex.unlock();

        // printf("#osp:mpi(%2i): shutting down recv thread\n",g->rank);fflush(0);
        // g->recvMutex.lock();
        g->recvThreadState = TERMINATED;
        g->recvTerminatedCond.broadcast();
        g->recvMutex.unlock();
      }

      void MultiIsendIrecvImpl::Group::shutdown()
      {
        printf("%i/%i triggering shutdown cond!\n",world.rank,world.size);fflush(0);

        // ------------------------------------------------------------------
        // shut down sending
        // ------------------------------------------------------------------
        printf("%i/%i triggering shutdown for send!\n",world.rank,world.size);fflush(0);
        sendMutex.lock();
        sendThreadState = FLAGGED_TO_TERMINATE;
        sendCond.broadcast();
        // and wait for shut down to happen
        while (sendThreadState != TERMINATED)
          sendTerminatedCond.wait(sendMutex);
        sendMutex.unlock();

        printf("%i/%i triggering shutdown for recv!\n",world.rank,world.size);fflush(0);
        // ------------------------------------------------------------------
        // shut down recving
        // ------------------------------------------------------------------
        recvMutex.lock();
        recvThreadState = FLAGGED_TO_TERMINATE;
        recvCond.broadcast();
        // send one dummy message to ourselves, to make sure
        // 'something' gets received before that flag is checked
        MPI_Status status;
        MPI_Request request;
        char done = 1;
        assert(sendQueue.empty());
        MPI_CALL(Isend(&done,1,MPI_BYTE,rank,tag,comm,&request));
        // MPI_CALL(Cancel(&request));
        // and wait for shut down to happen
        while (recvThreadState != TERMINATED)
          recvTerminatedCond.wait(recvMutex);
        recvMutex.unlock();
      }

      void MultiIsendIrecvImpl::init()
      {
        mpi::world.barrier();
        printf("#osp:mpi:MultiIsendIrecvMessaging started up %i/%i\n",mpi::world.rank,mpi::world.size);
        fflush(0);
        mpi::world.barrier();
      }

      void MultiIsendIrecvImpl::shutdown()
      {
        mpi::world.barrier();
        printf("#osp:mpi:MultiIsendIrecvMessaging shutting down %i/%i\n",mpi::world.rank,mpi::world.size);
        fflush(0);
        mpi::world.barrier();
        for (int i=0;i<myGroups.size();i++)
          myGroups[i]->shutdown();

        MPI_CALL(Finalize());
      }

      async::Group *MultiIsendIrecvImpl::createGroup(const std::string &name, 
                                                    MPI_Comm comm, 
                                                    Consumer *consumer, 
                                                    int32 tag)
      {
        Group *g = new Group(name,comm,consumer,tag);
        myGroups.push_back(g);
        return g;
      }

      void MultiIsendIrecvImpl::send(const Address &dest, void *msgPtr, int32 msgSize)
      {
        QueuedMessage *msg = new QueuedMessage;
        msg->addr = dest;
        msg->ptr  = msgPtr;
        msg->size = msgSize;

        Group *g = (Group *)dest.group;
        g->sendMutex.lock();
        if (msgSize == 4) { 
          printf("%i: added new 4-byte msg to queue, tgt = %i\n",world.rank,msg->addr.rank);
          dbg = true;
        }
        // if (g->sendQueue.empty())
        //   g->sendCond.broadcast();
        g->sendQueue.push_back(msg);
        g->sendCond.broadcast();
        g->sendMutex.unlock();
      }

    } // ::ospray::mpi::async
  } // ::ospray::mpi
} // ::ospray
