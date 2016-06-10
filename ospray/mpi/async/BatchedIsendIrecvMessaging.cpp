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
          sendThread(this), recvThread(this), procThread(this)
      {
        recvThread.start();
        sendThread.start();
        procThread.start();
      }

      // TODO WILL: Log send bandwidth as well
      void BatchedIsendIrecvImpl::SendThread::run()
      {
        using namespace std::chrono;
        // TODO WILL: Just estimate bandwidth here instead of going through MPI_Finalize
        // TODO: PRINT BASED ON FRAME TIMING
        const milliseconds measureTime(500);
        milliseconds elapsedTime(0);
        size_t bytesSent = 0;
        
        Group  *g = this->group;
        Action *actions[SEND_WINDOW_SIZE];
        MPI_Request request[SEND_WINDOW_SIZE];
        while (1) {
          // usleep(80);
          size_t numActions = g->sendQueue.getSome(actions,SEND_WINDOW_SIZE);

          auto startTime = high_resolution_clock::now();

          for (int i=0;i<numActions;i++) {
            Action *action = actions[i];
            bytesSent += action->size;
            MPI_CALL(Isend(action->data,action->size,MPI_BYTE,
                           action->addr.rank,g->tag,g->comm,&request[i]));
          }
          
          MPI_CALL(Waitall(numActions,request,MPI_STATUSES_IGNORE));

          auto endTime = high_resolution_clock::now();
          elapsedTime += duration_cast<milliseconds>(endTime - startTime);
          if (elapsedTime >= measureTime) {
            auto elapsedSec = duration_cast<duration<double, std::ratio<1>>>(elapsedTime);
            double gbitSent = bytesSent * 8e-9;
            std::cout << "Worker " << mpi::worker.rank << " send bandwidth "
              << gbitSent / elapsedSec.count()
              << " Gbps over " << elapsedSec.count() << "s\n";
            elapsedTime = milliseconds(0);
            bytesSent = 0;
          }
          
          for (int i=0;i<numActions;i++) {
            Action *action = actions[i];
            free(action->data);
            delete action;
          }
        }
      }
      
      void BatchedIsendIrecvImpl::RecvThread::run()
      {
        using namespace std::chrono;

        // note this thread not only _probes_ for new receives, it
        // also immediately starts the receive operation using Irecv()
        Group *g = (Group *)this->group;

        MPI_Request request[RECV_WINDOW_SIZE];
        Action *actions[RECV_WINDOW_SIZE];
        MPI_Status status;
        int numRequests = 0;
        
        // TODO WILL: Just estimate bandwidth here instead of going through MPI_Finalize
        // TODO: PRINT BASED ON FRAME TIMING
        const milliseconds measureTime(500);
        milliseconds elapsedTime(0);
        size_t bytesRecvd = 0;
        
        while (1) {
          numRequests = 0;
          // wait for first message
          // TODO WILL: The probe is a blocking call until we get another message, this
          // shouldn't be accounted for in our timing
          // Or do we actually want it included? Since the time we spend waiting and not
          // sending would reduce our overall avg. bandwidth and should be accounted for
          auto startTime = high_resolution_clock::now();
          {
            // usleep(280);
            MPI_CALL(Probe(MPI_ANY_SOURCE,g->tag,g->comm,&status));

            Action *action = new Action;
            action->addr = Address(g,status.MPI_SOURCE);
            MPI_CALL(Get_count(&status,MPI_BYTE,&action->size));

            bytesRecvd += action->size;
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

            bytesRecvd += action->size;
            action->data = malloc(action->size);
            MPI_CALL(Irecv(action->data,action->size,MPI_BYTE,
                           status.MPI_SOURCE,status.MPI_TAG,
                           g->comm,&request[numRequests]));
            actions[numRequests++] = action;
          }

          // now, have certain number of messages available...
          MPI_CALL(Waitall(numRequests,request,MPI_STATUSES_IGNORE));
          // TODO WILL: Track the bytes transfered, after a total of 10s have elapsed print
          // out the bandwidth used (with our without the blocking probe in that timing?)
          
          auto endTime = high_resolution_clock::now();
          elapsedTime += duration_cast<milliseconds>(endTime - startTime);
          if (elapsedTime >= measureTime) {
            auto elapsedSec = duration_cast<duration<double, std::ratio<1>>>(elapsedTime);
            double gbitRecvd = bytesRecvd * 8e-9;
            std::cout << "Worker " << mpi::worker.rank << " recv bandwidth "
              << gbitRecvd / elapsedSec.count()
              << " Gbps over " << elapsedSec.count() << "s\n";
            elapsedTime = milliseconds(0);
            bytesRecvd = 0;
          }

          // OK, all actions are valid now
          g->recvQueue.putSome(&actions[0],numRequests);
        }
      }

      void BatchedIsendIrecvImpl::ProcThread::run()
      {
        Group *g = (Group *)this->group;
        while (1) {
          Action *actions[PROC_WINDOW_SIZE];
          size_t numActions = g->recvQueue.getSome(actions,PROC_WINDOW_SIZE);
          for (int i=0;i<numActions;i++) {
            Action *action = actions[i];
            g->consumer->process(action->addr,action->data,action->size);
            delete action;
          }
        }
      }

      void BatchedIsendIrecvImpl::Group::shutdown()
      {
        std::cout << "shutdown() not implemented for this messaging ..."
                  << std::endl;
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
