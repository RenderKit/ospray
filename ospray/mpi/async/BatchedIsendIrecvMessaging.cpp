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
// WILL HACK
#include "ospray/mpi/DistributedFrameBuffer.h"
#include <limits>
#include <atomic>

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

      using namespace std::chrono;

      enum BandwidthType {
        SEND, RECV
      };

      std::ostream& operator<<(std::ostream &os, const BandwidthType &t){
        switch (t) {
          case SEND: os << "send"; break;
          case RECV: os << "recv"; break;
          default: os << "INVALID!?"; break;
        }
        return os;
      }

      struct BandwidthProfiler {
        milliseconds elapsedTime = milliseconds(0);
        // Switch these to not be *Sent just minBytes, maxBytes w/e
        size_t bytesXfd = 0;
        size_t minBytes = std::numeric_limits<size_t>::max();
        size_t maxBytes = 0;
        size_t avgBytes = 0;
        size_t numMsg = 0;
        size_t numAsync = 0;
        size_t frameStart = 0;
        size_t frameEnd = 0;
        BandwidthType type;

        BandwidthProfiler(BandwidthType type) : type(type){}

        void print() {
          auto elapsedSec = duration_cast<duration<double, std::ratio<1>>>(elapsedTime);
          double gbitSent = bytesXfd * 8e-9;
          std::cout << "Worker " << mpi::worker.rank << " " << type << " bandwidth "
            << gbitSent / (frameEnd - frameStart) << " Gbit/frame over "
            << frameEnd - frameStart << " frames\n";

          if (numMsg > 0 && numAsync > 0) {
            std::cout << "Worker " << mpi::worker.rank << " avg async " << type
              << " msgs " << static_cast<double>(numMsg) / numAsync << " w/ total msgs = "
              << numMsg << "\n";
          }


          elapsedTime = milliseconds(0);
          bytesXfd = 0;
          minBytes = std::numeric_limits<size_t>::max();
          maxBytes = 0;
          avgBytes = 0;
          numMsg = 0;
          numAsync = 0;
          frameStart = 0;
          frameEnd = 0;
        }
      };

      // TODO WILL: Log send bandwidth as well
      void BatchedIsendIrecvImpl::SendThread::run()
      {
        BandwidthProfiler bprof(SEND);
        bprof.frameStart = DistributedFrameBuffer::dbgCurrentFrame.load();
        
        Group  *g = this->group;
        Action *actions[SEND_WINDOW_SIZE];
        MPI_Request request[SEND_WINDOW_SIZE];
        while (1) {
          // usleep(80);
          // Note that this is a blocking call
          size_t numActions = g->sendQueue.getSome(actions,SEND_WINDOW_SIZE);

          // NOTE: We may be off by a little bit if the frame switches right when we check this but we've
          // not sent all it's data? However we should't be off by that much I think
          size_t currentFrame = DistributedFrameBuffer::dbgCurrentFrame.load();
          if (currentFrame - bprof.frameStart >= 25){
            bprof.frameEnd = currentFrame;
            bprof.print();
            bprof.frameStart = DistributedFrameBuffer::dbgCurrentFrame.load();

          }

          for (int i=0;i<numActions;i++) {
            Action *action = actions[i];

            bprof.bytesXfd += action->size;
            bprof.minBytes = std::min(bprof.minBytes, size_t(action->size));
            bprof.maxBytes = std::max(bprof.maxBytes, size_t(action->size));
            bprof.avgBytes = (action->size + bprof.avgBytes * bprof.numMsg) / (bprof.numMsg + 1);
            ++bprof.numMsg;

            MPI_CALL(Isend(action->data,action->size,MPI_BYTE,
                           action->addr.rank,g->tag,g->comm,&request[i]));
          }
          ++bprof.numAsync;
          
          // TODO: Batch up messages into a buffer and then fill it as messages are
          // sent only call waitall when the buffer is filled and we must flush.
          // May need to maintain multiple output buffers for different ranks to send too
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
        
        BandwidthProfiler bprof(RECV);
        bprof.frameStart = DistributedFrameBuffer::dbgCurrentFrame.load();
        
        while (1) {
          numRequests = 0;

          // NOTE: We may be off by a little bit if the frame switches right when we check this but we've
          // not sent all it's data? However we should't be off by that much I think
          size_t currentFrame = DistributedFrameBuffer::dbgCurrentFrame.load();
          if (currentFrame - bprof.frameStart >= 25){
            bprof.frameEnd = currentFrame;
            bprof.print();
            bprof.frameStart = DistributedFrameBuffer::dbgCurrentFrame.load();

          }

          // wait for first message
          // TODO WILL: The probe is a blocking call until we get another message, this
          // shouldn't be accounted for in our timing
          // Or do we actually want it included? Since the time we spend waiting and not
          // sending would reduce our overall avg. bandwidth and should be accounted for
          {
            // usleep(280);
            MPI_CALL(Probe(MPI_ANY_SOURCE,g->tag,g->comm,&status));

            Action *action = new Action;
            action->addr = Address(g,status.MPI_SOURCE);
            MPI_CALL(Get_count(&status,MPI_BYTE,&action->size));

            bprof.bytesXfd += action->size;
            bprof.minBytes = std::min(bprof.minBytes, size_t(action->size));
            bprof.maxBytes = std::max(bprof.maxBytes, size_t(action->size));
            bprof.avgBytes = (action->size + bprof.avgBytes * bprof.numMsg) / (bprof.numMsg + 1);
            ++bprof.numMsg;

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

            bprof.bytesXfd += action->size;
            bprof.minBytes = std::min(bprof.minBytes, size_t(action->size));
            bprof.maxBytes = std::max(bprof.maxBytes, size_t(action->size));
            bprof.avgBytes = (action->size + bprof.avgBytes * bprof.numMsg) / (bprof.numMsg + 1);
            ++bprof.numMsg;

            action->data = malloc(action->size);
            MPI_CALL(Irecv(action->data,action->size,MPI_BYTE,
                           status.MPI_SOURCE,status.MPI_TAG,
                           g->comm,&request[numRequests]));
            actions[numRequests++] = action;
          }
          ++bprof.numAsync;

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
