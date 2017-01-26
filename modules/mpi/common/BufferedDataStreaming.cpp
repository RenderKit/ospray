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
      // std::cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << std::endl;
      // std::cout << "BCAST: trying to get lock for mpi bcast read, has=" << whoHasTheMPILock() << std::endl;
      lockMPI("MPIBcastFabric::read");
      // std::cout << "#mpi: BCAST: starting receive of bcast " << " on comm " << (int*)group.comm  << std::endl << std::flush;
      // printf("BCAST 2nd message\n"); fflush(0);
      MPI_CALL(Bcast(&sz32,1,MPI_INT,0,group.comm));
      // std::cout << "#mpi: BCAST received bcast size from sender; size is " << sz32 << " on comm " << (int*)group.comm << std::endl;


      // std::cout << "BCAST receiver reaching barrier" << std::endl;
      MPI_CALL(Barrier(group.comm));
      // std::cout << "BCAST receiver passed barrier" << std::endl;

      // PRINT(sz32);
      buffer = new uint8_t[sz32];
      // std::cout << "#mpi: BCAST starting receive of size " << sz32 << std::endl;
      MPI_CALL(Bcast(buffer,sz32,MPI_BYTE,0,group.comm));
      // std::cout << "#mpi: BCAST done receiving MEM of size " << sz32 << std::endl;
      unlockMPI();
      mem = buffer;
      return sz32;
    }

    /*! send exact number of bytes - the fabric can do that through
      multiple smaller messages, but all bytes have to be
      delivered */
    void   MPIBcastFabric::send(void *mem, size_t size) 
    {
      // PING; PRINT((int*)this);
      
      assert(size < (1LL<<30));
      uint32_t sz32 = size;
      // PING;
      // PING; std::cout << "locking MPI!!!" << std::endl;
      // PRINT(whoHasTheMPILock());
      lockMPI("MPIBcastFabric::send");
      // PRINT((int*)MPI_COMM_WORLD);
      // PRINT((int*)group.comm);
      // PRINT(group.rank);
      // PRINT(group.size);
      // std::cout << "#mpi: BCAST: SENDING size of " << sz32 << " on comm " << (int*)group.comm << std::endl;
      MPI_CALL(Bcast(&sz32,1,MPI_INT,MPI_ROOT,group.comm));
      // std::cout << "#mpi: BCAST: _done_ SENDING size of " << sz32 << " on comm " << (int*)group.comm << std::endl;
      // std::cout << "BCAST sender reaching barrier" << std::endl;
      MPI_CALL(Barrier(group.comm));
      // std::cout << "BCAST sender passed barrier" << std::endl;
      // PRINT(sz32);
      // std::cout << "#mpi: BCAST sending MEM of size " << sz32 << std::endl;
      MPI_CALL(Bcast(mem,sz32,MPI_BYTE,MPI_ROOT,group.comm));
      // std::cout << "#mpi: BCAST done sending MEM of size " << sz32 << std::endl;
      unlockMPI();
    }

    
    void BufferedFabric::ReadStream::read(void *mem, size_t size)
    {
      uint8_t *writePtr  = (uint8_t*)mem;
      size_t   numStillMissing = size;

      // std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
      // PING;
      // PRINT(size);
      while (numStillMissing > 0) {
        // fill in what we already have
        size_t numWeCanDeliver = std::min(numAvailable,numStillMissing);
        if (numWeCanDeliver == 0) {
          // read some more ... we HAVE to fulfill this 'read()'
          // request, so have to read here
          numAvailable = fabric->read((void*&)buffer);
          // PRINT(numAvailable);
          continue;
        }
        
        memcpy(writePtr,buffer,numWeCanDeliver);
        buffer += numWeCanDeliver;
        numStillMissing -= numWeCanDeliver;
        numAvailable    -= numWeCanDeliver;
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
        fabric->send(buffer,numInBuffer);
      numInBuffer = 0;
    };

  } // ::ospray::mpi
} // ::ospray
