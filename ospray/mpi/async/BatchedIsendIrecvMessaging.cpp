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

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h> // for Sleep
#else
#  include <time.h>
#endif
#include <chrono>
#include <atomic>
#include <thread>
#include "BatchedIsendIrecvMessaging.h"

namespace ospray {
  namespace mpi {
    namespace async {

      enum { SEND_WINDOW_SIZE = 48 };
      enum { RECV_WINDOW_SIZE = 48 };
      enum { PROC_WINDOW_SIZE = 20 };

      BatchedIsendIrecvImpl::Group::Group(const std::string &name, MPI_Comm comm, 
                                       Consumer *consumer, int32 tag)
        : async::Group(comm,consumer,tag),
          sendThread(this), recvThread(this), procThread(this), shouldExit(false)
      {
        recvThread.start();
        sendThread.start();
        procThread.start();
      }

      void BatchedIsendIrecvImpl::SendThread::run()
      {
        Group  *g = this->group;
        Action *actions[SEND_WINDOW_SIZE];
        MPI_Request request[SEND_WINDOW_SIZE];
        while (1) {
          // usleep(80);
          size_t numActions = 0;
          while (numActions == 0){
            numActions = g->sendQueue.getSomeFor(actions,SEND_WINDOW_SIZE,
                std::chrono::milliseconds(1));
            if (g->shouldExit.load()){
              return;
            }
          }
          for (int i=0;i<numActions;i++) {
            Action *action = actions[i];
            MPI_CALL(Isend(action->data,action->size,MPI_BYTE,
                           action->addr.rank,g->tag,g->comm,&request[i]));
          }

          // TODO: Is it ok to wait even if we're exiting? Maybe we'd just get send
          // failed statuses back?
          MPI_CALL(Waitall(numActions,request,MPI_STATUSES_IGNORE));
          
          for (int i=0;i<numActions;i++) {
            Action *action = actions[i];
            free(action->data);
            delete action;
          }
        }
      }
      
      void BatchedIsendIrecvImpl::RecvThread::run()
      {
        // note this thread not only _probes_ for new receives, it
        // also immediately starts the receive operation using Irecv()
        Group *g = (Group *)this->group;

        MPI_Request request[RECV_WINDOW_SIZE];
        Action *actions[RECV_WINDOW_SIZE];
        MPI_Status status;
        int numRequests = 0;
#ifdef _WIN32
        const DWORD sleep_time = 1; // ms --> much longer than 150us
#else
        const timespec sleep_time = timespec{0, 150000};
#endif

        while (1) {
          numRequests = 0;
          // wait for first message
          {
            // usleep(280);
            int msgAvail = 0;
            while (!msgAvail) {
              MPI_CALL(Iprobe(MPI_ANY_SOURCE,g->tag,g->comm,&msgAvail,&status));
              if (g->shouldExit.load()){
                return;
              }
              if (msgAvail){
                break;
              }
#ifdef _WIN32
              Sleep(sleep_time);
#else
              // TODO: Can we do a CMake feature test for this_thread::sleep_for and
              // conditionally use nanosleep?
              nanosleep(&sleep_time, NULL);
#endif
            }
            Action *action = new Action;
            action->addr = Address(g,status.MPI_SOURCE);
            MPI_CALL(Get_count(&status,MPI_BYTE,&action->size));

            action->data = malloc(action->size);
            MPI_CALL(Irecv(action->data,action->size,MPI_BYTE,
                           status.MPI_SOURCE,status.MPI_TAG,
                           g->comm,&request[numRequests]));
            actions[numRequests++] = action;
          }
          // ... and add all the other ones that are outstanding, up
          // to max window size
          while (numRequests < RECV_WINDOW_SIZE) {
            int msgAvail;
            MPI_CALL(Iprobe(MPI_ANY_SOURCE,g->tag,g->comm,&msgAvail,&status));
            if (!msgAvail)
              break;
            
            Action *action = new Action;
            action->addr = Address(g,status.MPI_SOURCE);
            MPI_CALL(Get_count(&status,MPI_BYTE,&action->size));

            action->data = malloc(action->size);
            MPI_CALL(Irecv(action->data,action->size,MPI_BYTE,
                           status.MPI_SOURCE,status.MPI_TAG,
                           g->comm,&request[numRequests]));
            actions[numRequests++] = action;
          }

          // TODO: Is it ok to wait even if we're exiting? Maybe we'd just get send
          // failed statuses back?
          // now, have certain number of messages available...
          MPI_CALL(Waitall(numRequests,request,MPI_STATUSES_IGNORE));

          // OK, all actions are valid now
          g->recvQueue.putSome(&actions[0],numRequests);
        }
      }

      void BatchedIsendIrecvImpl::ProcThread::run()
      {
        Group *g = (Group *)this->group;
        while (1) {
          Action *actions[PROC_WINDOW_SIZE];
          size_t numActions = 0;
          while (numActions == 0){
            numActions = g->recvQueue.getSomeFor(actions,PROC_WINDOW_SIZE,
                std::chrono::milliseconds(1));
            if (g->shouldExit.load()){
              return;
            }
          }
          for (int i=0;i<numActions;i++) {
            Action *action = actions[i];
            g->consumer->process(action->addr,action->data,action->size);
            delete action;
          }
        }
      }

      void BatchedIsendIrecvImpl::Group::shutdown()
      {
        std::cout << "#osp:mpi:BatchIsendIrecvMessaging:Group shutting down" << std::endl;
        shouldExit.store(true);
        sendThread.join();
        recvThread.join();
        procThread.join();
      }

      void BatchedIsendIrecvImpl::init()
      {
        mpi::world.barrier();
        printf("#osp:mpi:BatchedIsendIrecvMessaging started up %i/%i\n",
               mpi::world.rank,mpi::world.size);
        fflush(0);
        mpi::world.barrier();
      }

      void BatchedIsendIrecvImpl::shutdown()
      {
        mpi::world.barrier();
        printf("#osp:mpi:BatchedIsendIrecvMessaging shutting down %i/%i\n",
               mpi::world.rank,mpi::world.size);
        fflush(0);
        mpi::world.barrier();
        for (int i=0;i<myGroups.size();i++)
          myGroups[i]->shutdown();

        MPI_CALL(Finalize());
      }

      async::Group *BatchedIsendIrecvImpl::createGroup(const std::string &name, 
                                                    MPI_Comm comm, 
                                                    Consumer *consumer, 
                                                    int32 tag)
      {
        Group *g = new Group(name,comm,consumer,tag);
        myGroups.push_back(g);
        return g;
      }

      void BatchedIsendIrecvImpl::send(const Address &dest,
                                       void *msgPtr,
                                       int32 msgSize)
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
