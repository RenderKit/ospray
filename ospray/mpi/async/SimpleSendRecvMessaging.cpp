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

//#define PROFILE_MPI 1

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h> // for Sleep
#else
#  include <time.h>
#endif
#include <chrono>
#include <atomic>
#include <thread>
#include "SimpleSendRecvMessaging.h"

namespace ospray {
  namespace mpi {
    namespace async {

#if PROFILE_MPI
      int frameID = 0;
      bool logIt = 0;
      double T0;
      double t_recv, t_send;
      size_t b_recv, b_sent;

      struct MsgLog {
        int to, size;
        double begin, end;
      };
      std::vector<MsgLog> sendLog, recvLog;

      extern "C" void async_beginFrame() 
      {
        ++ frameID;
        if (frameID > 20) logIt = true;

        if (logIt) {
          T0 = getSysTime();
          t_send = 0;
          t_recv = 0;
          b_recv = 0;
          b_sent = 0;
          sendLog.clear();
          recvLog.clear();
        }
      }

      extern "C" void async_endFrame()
      {
        printf("rank %i t_send %fMb in %fs recv %fMb in %fs\n",
               mpi::world.rank,
               b_sent*1e-6f,t_send,b_recv*1e-6f,t_recv);
        for (int i=0;i<sendLog.size();i++) {
          MsgLog log = sendLog[i];
          printf(" sent to to %i (%fMB) from %f to %f (%fs)\n",log.to,log.size*1e-6f,
                 log.begin-T0,log.end-T0,log.end-log.begin);
        }
        for (int i=0;i<recvLog.size();i++) {
          MsgLog log = recvLog[i];
          printf(" recvd from %i (%fMB) from %f to %f (%fs)\n",log.to,log.size*1e-6f,
                 log.begin-T0,log.end-T0,log.end-log.begin);
        }
      }
#else
      extern "C" void async_beginFrame() {}
      extern "C" void async_endFrame() {}
#endif

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
          Action *action = nullptr;
          size_t numActions = 0;
          while (numActions == 0){
            numActions = g->sendQueue.getSomeFor(&action,1,std::chrono::milliseconds(1));
            if (g->shouldExit.load()){
              return;
            }
          }
          double t0 = getSysTime();
          MPI_CALL(Send(action->data,action->size,MPI_BYTE,
                        action->addr.rank,g->tag,action->addr.group->comm));
          double t1 = getSysTime();
#if PROFILE_MPI
          if (logIt) {
            t_send += (t1-t0);
            b_sent += action->size;

            MsgLog log;
            log.to = action->addr.rank;
            log.size = action->size;
            log.begin = t0;
            log.end = t1;
            sendLog.push_back(log);
          }
#endif
          free(action->data);
          delete action;
        }
      }

      void SimpleSendRecvImpl::RecvThread::run()
      {
        Group *g = (Group *)this->group;
#ifdef _WIN32
        const DWORD sleep_time = 1; // ms --> much longer than 150us
#else
        const timespec sleep_time = timespec{ 0, 150000 };
#endif

        while (1) {
          MPI_Status status;
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
          double t0 = getSysTime();
          MPI_CALL(Recv(action->data,action->size,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,
                        g->comm,MPI_STATUS_IGNORE));
          double t1 = getSysTime();
#if PROFILE_MPI
          if (logIt) {
            t_recv += (t1-t0);
            b_recv += action->size;

            MsgLog log;
            log.to = action->addr.rank;
            log.size = action->size;
            log.begin = t0;
            log.end = t1;
            recvLog.push_back(log);
          }
#endif
          g->procQueue.put(action);
        }
      }

      void SimpleSendRecvImpl::ProcThread::run()
      {
        Group *g = (Group *)this->group;
        while (1) {
          Action *action = nullptr;
          size_t numActions = 0;
          while (numActions == 0){
            numActions = g->procQueue.getSomeFor(&action,1,std::chrono::milliseconds(1));
            if (g->shouldExit.load()){
              return;
            }
          }
          g->consumer->process(action->addr,action->data,action->size);
          delete action;
        }
      }

      void SimpleSendRecvImpl::Group::shutdown()
      {
        std::cout << "#osp:mpi:SimpleSendRecvImpl:Group shutting down" << std::endl;
        shouldExit.store(true);
        sendThread.join();
        recvThread.join();
        procThread.join();
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
