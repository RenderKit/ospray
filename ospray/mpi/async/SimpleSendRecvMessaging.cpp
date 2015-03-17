// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "SimpleSendRecvMessaging.h"

namespace ospray {
  namespace mpi {
    namespace async {
      SimpleSendRecvImpl::Group::Group(const std::string &name, MPI_Comm comm, 
                                       Consumer *consumer, int32 tag)
        : async::Group(// name,
                       comm,consumer,tag),
          beginShutDown(0), sendIsShutDown(0), recvIsShutDown(0)
      {
        recvThread = embree::createThread(recvThreadFunc,this);
        sendThread = embree::createThread(sendThreadFunc,this);
      }

      void SimpleSendRecvImpl::Group::sendThreadFunc(void *arg)
      {
        Group *g = (Group *)arg;

        while (1) {
          g->mutex.lock();
          while (g->sendQueue.empty() && !g->beginShutDown)
            g->cond.wait(g->mutex);
          if (g->beginShutDown) {
            break;
          }

          QueuedMessage *msg = g->sendQueue.front();
          g->sendQueue.pop_front();
          g->mutex.unlock();

          MPI_CALL(Send(msg->ptr,msg->size,MPI_BYTE,
                        msg->addr.rank,g->tag,msg->addr.group->comm));
          free(msg->ptr);
          delete msg;
        }

        // printf("#osp:mpi(%2i): shutting down send thread\n",g->rank);fflush(0);
        g->sendIsShutDown = true;
        g->cond.broadcast();
        g->mutex.unlock();
      }

      void SimpleSendRecvImpl::Group::recvThreadFunc(void *arg)
      {
        Group *g = (Group *)arg;

        while (1) {
          MPI_Status status;
          MPI_CALL(Probe(MPI_ANY_SOURCE,g->tag,g->comm,&status));
          // printf("#osp:mpi(%2i): incoming from %i\n",g->rank,status.MPI_SOURCE);fflush(0);
          if (g->beginShutDown) {
            int done;
            MPI_CALL(Recv(&done,1,MPI_INT,status.MPI_SOURCE,status.MPI_TAG,g->comm,&status));
            break;
          }
          
          int count = -1;
          MPI_CALL(Get_count(&status,MPI_BYTE,&count));
          void *msg = malloc(count);
          MPI_CALL(Recv(msg,count,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,
                        g->comm,&status));
          g->consumer->process(Address(g,status.MPI_SOURCE),msg,count);
        }
        g->mutex.unlock();

        // printf("#osp:mpi(%2i): shutting down recv thread\n",g->rank);fflush(0);
        g->mutex.lock();
        g->recvIsShutDown = true;
        g->cond.broadcast();
        g->mutex.unlock();
      }

      void SimpleSendRecvImpl::Group::shutdown()
      {
        mutex.lock();

        // -------------------------------------------------------
        // mark shuttdown flag
        // -------------------------------------------------------
        beginShutDown = true;
        cond.broadcast();

        // -------------------------------------------------------
        // wait for send thread to terminate
        // -------------------------------------------------------
        while (!sendIsShutDown)
          cond.wait(mutex);

        // -------------------------------------------------------
        // send recv thread a shutdown message (to make sure it wakes
        // from its 'probe'), then wait for it to terminate
        // -------------------------------------------------------
        MPI_Status status;
        char done = 1;
        assert(sendQueue.empty());
        MPI_CALL(Send(&done,1,MPI_BYTE,rank,tag,comm));

        while (!recvIsShutDown) 
          cond.wait(mutex);

        mutex.unlock();
      }

      void SimpleSendRecvImpl::init()
      {
        mpi::world.barrier();
        printf("#osp:mpi:SimpleSendRecvMessaging started up %i/%i\n",mpi::world.rank,mpi::world.size);
        fflush(0);
        mpi::world.barrier();
      }

      void SimpleSendRecvImpl::shutdown()
      {
        mpi::world.barrier();
        printf("#osp:mpi:SimpleSendRecvMessaging shutting down %i/%i\n",mpi::world.rank,mpi::world.size);
        fflush(0);
        mpi::world.barrier();
        for (int i=0;i<myGroups.size();i++)
          myGroups[i]->shutdown();

        MPI_CALL(Finalize());
      }

      async::Group *SimpleSendRecvImpl::createGroup(const std::string &name, 
                                                    MPI_Comm comm, 
                                                    Consumer *consumer, 
                                                    int32 tag)
      {
        Group *g = new Group(name,comm,consumer,tag);
        myGroups.push_back(g);
        return g;
      }

      void SimpleSendRecvImpl::send(const Address &dest, void *msgPtr, int32 msgSize)
      {
        QueuedMessage *msg = new QueuedMessage;
        msg->addr = dest;
        msg->ptr  = msgPtr;
        msg->size = msgSize;

        Group *g = (Group *)dest.group;
        g->mutex.lock();
        g->sendQueue.push_back(msg);
        g->cond.broadcast();
        g->mutex.unlock();
      }

    } // ::ospray::mpi::async
  } // ::ospray::mpi
} // ::ospray
