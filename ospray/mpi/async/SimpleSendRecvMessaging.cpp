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

#include "SimpleSendRecvMessaging.h"

namespace ospray {
  namespace mpi {
    namespace async {
      SimpleSendRecvImpl::Group::Group(const std::string &name, MPI_Comm comm, 
                                       Consumer *consumer, int32 tag)
        : async::Group(comm,consumer,tag),
          sendThread(this), recvThread(this), procThread(this)
      {
        sendThread.start();
        recvThread.start();
        procThread.start();
      }

      void SimpleSendRecvImpl::SendThread::run()
      {
        Group *g = this->group;

        while (1) {
          Action *action = g->sendQueue.get();
          MPI_CALL(Send(action->data,action->size,MPI_BYTE,
                        action->addr.rank,g->tag,action->addr.group->comm));
          free(action->data);
          delete action;
        }
      }

      void SimpleSendRecvImpl::RecvThread::run()
      {
        Group *g = (Group *)this->group;

        while (1) {
          MPI_Status status;
          // PING;fflush(0);
          MPI_CALL(Probe(MPI_ANY_SOURCE,g->tag,g->comm,&status));
          Action *action = new Action;
          action->addr = Address(g,status.MPI_SOURCE);

          MPI_CALL(Get_count(&status,MPI_BYTE,&action->size));

          action->data = malloc(action->size);
          MPI_CALL(Recv(action->data,action->size,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,
                        g->comm,MPI_STATUS_IGNORE));
          g->procQueue.put(action);
        }
      }

      void SimpleSendRecvImpl::ProcThread::run()
      {
        Group *g = (Group *)this->group;
        while (1) {
          Action *action = g->procQueue.get();
          g->consumer->process(action->addr,action->data,action->size);
          delete action;
        }
      }

      void SimpleSendRecvImpl::Group::shutdown()
      {
        std::cout << "shutdown() not implemented for this messaging ..." << std::endl;
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
        Action *action = new Action;
        action->addr   = dest;
        action->data   = msgPtr;
        action->size   = msgSize;

        Group *g = (Group *)dest.group;
        g->sendQueue.put(action);
      }

    } // ::ospray::mpi::async
  } // ::ospray::mpi
} // ::ospray
