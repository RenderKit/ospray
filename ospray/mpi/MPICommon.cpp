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

#include "mpi/MPICommon.h"
#include "mpi/async/CommLayer.h"

namespace ospray {
  namespace mpi {

    OSPRAY_INTERFACE Group world;
    OSPRAY_INTERFACE Group app;
    OSPRAY_INTERFACE Group worker;

    void init(int *ac, const char **av)
    {
      int initialized = false;
      MPI_CALL(Initialized(&initialized));

      if (!initialized) {
        // MPI_Init(ac,(char ***)&av);
        int required = MPI_THREAD_MULTIPLE;
        int provided = 0;
        MPI_CALL(Init_thread(ac,(char ***)&av,required,&provided));
        if (provided != required)
          throw std::runtime_error("MPI implementation does not offer multi-threading capabilities");
      }
      else
      {
        printf("running ospray in pre-initialized mpi mode\n");
        int provided;
        MPI_Query_thread(&provided);
        int requested = MPI_THREAD_MULTIPLE;
        if (provided != requested)
          throw std::runtime_error("ospray requires mpi to be initialized with "
            "MPI_THREAD_MULTIPLE if initialized before calling ospray");
      }
      world.comm = MPI_COMM_WORLD;
      MPI_CALL(Comm_rank(MPI_COMM_WORLD,&world.rank));
      MPI_CALL(Comm_size(MPI_COMM_WORLD,&world.size));

      mpi::async::CommLayer::WORLD = new mpi::async::CommLayer;
      mpi::async::Group *worldGroup =
          mpi::async::createGroup(MPI_COMM_WORLD,
                                  mpi::async::CommLayer::WORLD,
                                  290374);
      mpi::async::CommLayer::WORLD->group = worldGroup;
    }


    //TODO: put in management class
    work::SerialBuffer sendBuffer(1024*2*16);
    Address sendAddress;
    size_t sendSizeIndex=0;
    size_t sendWorkIndex=0;
    int sendNumMessages = 0;
    void send(const Address& addr, work::Work* work)
    {
      work::SerialBuffer& buf = sendBuffer;
      int indicator = -1;
      // MPI_CALL(Bcast(&indicator, 1, MPI_INT, MPI_ROOT, mpi::worker.comm));
      // The packed buffer format is:
      // size_t: size of message
      // int: num works
      // [(size_t, X)...]: message tag headers followed by their data
      if (buf.getIndex() == 0)
      {
        buf << indicator;
        sendSizeIndex = buf.getIndex();
        buf << size_t(0) << int(1);
        sendWorkIndex = buf.getIndex();
      }
      buf  << work->getTag() << *work;
      sendNumMessages++;
      // TODO: This is assuming we're always sending to the same place
      if (sendAddress.group != NULL && sendAddress != addr) {
        throw std::runtime_error("Can't send to different addresses currently!");
      }
      sendAddress = addr;
      // flush();
    }

    void internalSendBuffer(const Address& addr, work::SerialBuffer& buf)
    {
      if (sendNumMessages == 0)
        return;
      size_t endIndex = buf.getIndex();
      size_t sz = buf.getIndex()-sendWorkIndex;
      // set size in msg header
      buf.setIndex(sendSizeIndex);
      PRINT(sendSizeIndex);
      buf << sz << sendNumMessages;
      buf.setIndex(endIndex);
      PRINT(sz);
      PRINT(sendNumMessages);
      // MPI_CALL(Bcast(&sz, 1, MPI_AINT, MPI_ROOT, mpi::worker.comm));
      // Now send the buffer
      if (addr.rank != SEND_ALL)
        std::runtime_error("mpi::send to individuals not yet implemented");
      MPI_CALL(Bcast(buf.getData(), 2048, MPI_BYTE, MPI_ROOT, addr.group->comm));
      if (sz > 2048-12)
        MPI_CALL(Bcast(buf.getPtr(sendWorkIndex), sz, MPI_BYTE, MPI_ROOT, addr.group->comm));
      buf.clear();
      sendNumMessages=0;
    }

    work::SerialBuffer recvBuffer(1024*2*16);
    void recv(const Address& addr, std::vector<work::Work*>& workCommands)
    {
      work::SerialBuffer& buf = recvBuffer;
      //TODO: assert current mode is syncronized
      int recvNumMessages = 0;
      size_t bufSize = 0;
      if (addr.rank != RECV_ALL)
        std::runtime_error("mpi::recv on individual ranks not yet implemented");
      MPI_Bcast(buf.getData(),2048,MPI_BYTE,0,addr.group->comm);
      int command = 0;
      buf >> command >> bufSize >> recvNumMessages;
      PRINT(bufSize);
      assert(command == -1 && "error fetching work, mpi out of sync");
      if (bufSize > (2048-12))
      {
        std::cout << "need more room for big msg" << std::endl;
        buf.clear();
        buf.reserve(bufSize);
        MPI_Bcast(buf.getData(),bufSize,MPI_BYTE,0,addr.group->comm);
      }
      work::decode_buffer(buf, workCommands, recvNumMessages);
      buf.clear();
    }

    // void send(const Address& address, void* msgPtr, int32 msgSize)
    // {
    //   mpi::async::CommLayer::WORLD->sendTo(address, msgPtr, msgSize);
    // }

    void flush()
    {
      internalSendBuffer(sendAddress, sendBuffer);
    }

    void barrier(const Group& group) { flush(); MPI_Barrier(group.comm); }

  } // ::ospray::mpi
} // ::ospray
