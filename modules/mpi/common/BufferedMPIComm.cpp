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

#include "BufferedMPIComm.h"

namespace ospray {
  namespace mpi {
    // The max size for an MPI Bcast is actually 2GB, but we cut it at 1.8GB
    const size_t BufferedMPIComm::MAX_BCAST = 1800000000LL;
    std::shared_ptr<BufferedMPIComm> BufferedMPIComm::global = nullptr;
    std::mutex BufferedMPIComm::globalCommAlloc;
    BufferedMPIComm::BufferedMPIComm(size_t bufSize) : sendBuffer(bufSize), recvBuffer(bufSize){}
    BufferedMPIComm::~BufferedMPIComm() {
      flush();
    }
    void BufferedMPIComm::send(const Address& addr, work::Work* work) {
      {
        std::lock_guard<std::mutex> lock(sendMutex);
        // The packed buffer format is:
        // size_t: size of message
        // int: number of work units being sent
        // [(size_t, X)...]: message tag headers followed by their data
        if (sendBuffer.getIndex() == 0) {
          sendSizeIndex = sendBuffer.getIndex();
          sendBuffer << size_t(0) << int(1);
          sendWorkIndex = sendBuffer.getIndex();
        }
        sendBuffer << work->getTag() << *work;
        sendNumMessages++;
      }

      // TODO: This is assuming we're always sending to the same place
      if (sendAddress.group != NULL && sendAddress != addr) {
        throw std::runtime_error("Can't send to different addresses currently!");
      }
      // TODO WILL: Instead of making data/setregion commands flushing based on their
      // size we should flush here if the sendbuffer has reached a certain size.
      sendAddress = addr;
      if (sendBuffer.getIndex() >= MAX_BCAST) {
        flush();
      }
    }
    void BufferedMPIComm::recv(const Address& addr, std::vector<work::Work*>& work) {
      //TODO: assert current mode is syncronized
      int recvNumMessages = 0;
      size_t bufSize = 0;
      if (addr.rank != RECV_ALL) {
        std::runtime_error("mpi::recv on individual ranks not yet implemented");
      }
      MPI_Bcast(recvBuffer.getData(), 2048, MPI_BYTE, 0, addr.group->comm);
      recvBuffer >> bufSize >> recvNumMessages;

      if (bufSize > 2048) {
        recvBuffer.reserve(bufSize);
        // If we have more than 2KB receive the data in batches as big as we can send with bcast
        for (size_t i = 2048; i < bufSize;) {
          const int to_recv = std::min(bufSize - i, MAX_BCAST);
          MPI_CALL(Bcast(recvBuffer.getPtr(i), to_recv, MPI_BYTE, 0, addr.group->comm));
          i += to_recv;
        }
      }
      recvBuffer.setIndex(sizeof(int) + sizeof(size_t));
      work::decode_buffer(recvBuffer, work, recvNumMessages);
      recvBuffer.clear();
    }
    void BufferedMPIComm::flush() {
      send(sendAddress, sendBuffer);
    }
    void BufferedMPIComm::barrier(const Group& group) {
      flush();
      MPI_Barrier(group.comm);
    }
    void BufferedMPIComm::send(const Address& addr, work::SerialBuffer& buf) {
      if (sendNumMessages == 0) {
        return;
      }
      std::lock_guard<std::mutex> lock(sendMutex);
      const size_t sz = buf.getIndex();
      // set size in msg header
      buf.setIndex(sendSizeIndex);
      buf << sz << sendNumMessages;
      buf.setIndex(sz);

      // Now send the buffer
      if (addr.rank != SEND_ALL) {
        std::runtime_error("mpi::send to individuals not yet implemented");
      }
      MPI_CALL(Bcast(buf.getData(), 2048, MPI_BYTE, MPI_ROOT, addr.group->comm));

      // If we have more than 2KB send the data in batches as big as we can send with bcast
      for (size_t i = 2048; i < sz;) {
        const int to_send = std::min(sz - i, MAX_BCAST);
        MPI_CALL(Bcast(buf.getPtr(i), to_send, MPI_BYTE, MPI_ROOT, addr.group->comm));
        i += to_send;
      }
      buf.clear();
      sendNumMessages = 0;
    }
    std::shared_ptr<BufferedMPIComm> BufferedMPIComm::get() {
      // Create the global buffered comm if we haven't already or if it's been
      // destroyed and we need it again.
      if (!global || global.use_count() == 0) {
        std::lock_guard<std::mutex> lock(globalCommAlloc);
        // Did someone else already allocate it by the time we got the mutex?
        if (!global || global.use_count() == 0) {
          global = std::make_shared<BufferedMPIComm>();
        }
      }
      return global;
    }
  }
}

