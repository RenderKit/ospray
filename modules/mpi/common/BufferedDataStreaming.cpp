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

    void BufferedFabric::ReadStream::read(void *mem, size_t size)
    {
      uint8_t *writePtr  = (uint8_t*)mem;
      size_t   numStillMissing = size;
          
      while (numStillMissing > 0) {
        // fill in what we already have
        size_t numWeCanDeliver = std::min(numAvailable,numStillMissing);
        memcpy(writePtr,buffer,numWeCanDeliver);
        buffer += numWeCanDeliver;
        numStillMissing -= numWeCanDeliver;
        numAvailable    -= numWeCanDeliver;

        if (numStillMissing > 0)
          // read some more ... we HAVE to fulfill this 'read()'
          // request, so have to read here
          numAvailable = fabric->read((void*&)buffer);
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
