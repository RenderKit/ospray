// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include <chrono>
#include <thread>

#include "MPICommon.h"
#include "BufferedDataStreaming.h"

namespace ospray {
  namespace mpi {

    /*! constructor - create a new broascast fabric that uses the given communicator */
    MPIBcastFabric::MPIBcastFabric(const mpi::Group &group)
      : group(group),
        buffer(nullptr)
    {
      if (!group.valid())
        throw std::runtime_error("#osp:mpi: trying to set up a MPI fabric "
                                 "with a invalid MPI communicator");
    }

    /*! receive some block of data - whatever the sender has sent -
      and give us size and pointer to this data */
    size_t MPIBcastFabric::read(void *&mem) 
    {
      if (buffer) delete[] buffer;

      uint32_t sz32 = 0;
      // Get the size of the bcast being sent to us. Only this part must be non-blocking,
      // after getting the size we know everyone will enter the blocking barrier and the
      // blocking bcast where the buffer is sent out.
      MPI_Request request;
      SERIALIZED_MPI_CALL(Ibcast(&sz32, 1, MPI_INT, 0, group.comm, &request));

      // Now do non-blocking test to see when this bcast is satisfied to avoid
      // locking out the send/recv threads
      int size_bcast_done = 0;
      while (!size_bcast_done) {
        SERIALIZED_MPI_CALL(Test(&request, &size_bcast_done, MPI_STATUS_IGNORE));
        if (!size_bcast_done) {
          std::this_thread::sleep_for(std::chrono::nanoseconds(250));
        }
      }

      buffer = new uint8_t[sz32];
      serialized(CODE_LOCATION, [&]() {
        MPI_CALL(Barrier(group.comm));
        MPI_CALL(Bcast(buffer, sz32, MPI_BYTE, 0, group.comm));
      });
      mem = buffer;
      return sz32;
    }

    /*! send exact number of bytes - the fabric can do that through
      multiple smaller messages, but all bytes have to be
      delivered */
    void MPIBcastFabric::send(void *mem, size_t size) 
    {
      assert(size < (1LL<<30));
      uint32_t sz32 = size;

      // Send the size of the bcast we're sending. Only this part must be non-blocking,
      // after getting the size we know everyone will enter the blocking barrier and the
      // blocking bcast where the buffer is sent out.
      MPI_Request request;
      SERIALIZED_MPI_CALL(Ibcast(&sz32, 1, MPI_INT, MPI_ROOT, group.comm, &request));

      // Now do non-blocking test to see when this bcast is satisfied to avoid
      // locking out the send/recv threads
      int size_bcast_done = 0;
      while (!size_bcast_done) {
        SERIALIZED_MPI_CALL(Test(&request, &size_bcast_done, MPI_STATUS_IGNORE));
        if (!size_bcast_done) {
          std::this_thread::sleep_for(std::chrono::nanoseconds(250));
        }
      }

      serialized(CODE_LOCATION, [&]() {
        MPI_CALL(Barrier(group.comm));
        MPI_CALL(Bcast(mem, sz32, MPI_BYTE, MPI_ROOT, group.comm));
      });
    }

    void BufferedFabric::ReadStream::read(void *mem, size_t size)
    {
      uint8_t *writePtr  = (uint8_t*)mem;
      size_t   numStillMissing = size;

      /*! note this code does not - ever - free the 'buffer' because
        that buffer is internal to the fabric. it's the fabric that
        returns this buffer, and that may or may not free it upon the
        next 'read()' */
      while (numStillMissing > 0) {
        // fill in what we already have
        size_t numWeCanDeliver = std::min(numAvailable,numStillMissing);
        if (numWeCanDeliver == 0) {
          // read some more ... we HAVE to fulfill this 'read()'
          // request, so have to read here
          numAvailable = fabric.get().read((void*&)buffer);
          continue;
        }
        
        memcpy(writePtr,buffer,numWeCanDeliver);
        numStillMissing -= numWeCanDeliver;
        numAvailable    -= numWeCanDeliver;
        writePtr        += numWeCanDeliver;
        buffer          += numWeCanDeliver;
      }
    }

    void BufferedFabric::WriteStream::write(void *mem, size_t size)
    {
      size_t stillToWrite = size;
      uint8_t *readPtr = (uint8_t*)mem;
      while (stillToWrite) {
        size_t numWeCanWrite = std::min(stillToWrite,size_t(maxBufferSize-numInBuffer));
        memcpy(buffer+numInBuffer,readPtr,numWeCanWrite);

        readPtr += numWeCanWrite;
        numInBuffer += numWeCanWrite;
        stillToWrite -= numWeCanWrite;

        if (numInBuffer == maxBufferSize)
          flush();
      }
    }

    void BufferedFabric::WriteStream::flush()
    {
      if (numInBuffer > 0)
        fabric.get().send(buffer,numInBuffer);
      numInBuffer = 0;
    };

  } // ::ospray::mpi
} // ::ospray
