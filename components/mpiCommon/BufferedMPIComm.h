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

#include <memory>
#include <mpi.h>
#include "common/OSPCommon.h"
#include "mpiCommon/SerialBuffer.h"

namespace ospray {
  namespace mpi {
    // Mangement class for the MPI send/recv buffers. Objects interesting
    // in sending/receiving messages through the MPI layer must go through this
    // object to easily buffer operations.
    class OSPRAY_MPI_INTERFACE BufferedMPIComm {
      const static size_t MAX_BCAST;
      // TODO: Sending to multiple addresses
      work::SerialBuffer sendBuffer;
      Address sendAddress;
      // TODO WILL: I think this sendsizeindex is redundant, it will always be 0
      size_t sendSizeIndex = 0;
      // TODO WILL: Won't the send work index always be 12?
      size_t sendWorkIndex = 0;
      int sendNumMessages = 0;
      work::SerialBuffer recvBuffer;
      std::mutex sendMutex;

      // TODO: Do we really want to go through a singleton for this?
      // I guess it makes it easiest to provide global batching of all messages.
      static std::shared_ptr<BufferedMPIComm> global;
      static std::mutex globalCommAlloc;

    public:
      BufferedMPIComm(size_t bufSize = 1024 * 2 * 16);
      ~BufferedMPIComm();
      // Send a work unit message to some address.
      // TODO: Sending to multiple addresses
      void send(const Address& addr, work::Work* work);
      // Recieve a work unit message from some address, filling the work
      // vector with the received work units.
      void recv(const Address& addr, std::vector<work::Work*>& work);
      // Flush the current send buffer.
      void flush();
      // Perform a barrier on the passed MPI Group.
      void barrier(const Group& group);
      // The management class works through a shared ptr global so anyone
      // using it can clone the ptr and keep it alive as long as needed.
      static std::shared_ptr<BufferedMPIComm> get();

    private:
      // Actually send the data in the buffer to the address specified.
      void send(const Address& addr, work::SerialBuffer& buf);
    };
  }
}

