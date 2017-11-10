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

#include "BufferedDataStreaming.h"

namespace ospcommon {
  namespace networking {

    BufferedReadStream::BufferedReadStream(Fabric &fabric)
      : fabric(fabric), buffer(nullptr), numAvailable(0)
    {
    }

    void BufferedReadStream::read(void *mem, size_t size)
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

    BufferedWriteStream::BufferedWriteStream(Fabric &fabric,
                                             size_t maxBufferSize)
      : fabric(fabric),
        buffer(new byte_t[maxBufferSize]),
        maxBufferSize(maxBufferSize),
        numInBuffer(0)
    {
    }

    BufferedWriteStream::~BufferedWriteStream()
    {
      delete [] buffer;
    }

    void BufferedWriteStream::write(const void *mem, size_t size)
    {
      size_t stillToWrite = size;
      auto *readPtr = (const uint8_t*)mem;
      while (stillToWrite) {
        size_t numWeCanWrite = std::min(stillToWrite,
                                        size_t(maxBufferSize-numInBuffer));
        memcpy(buffer+numInBuffer,readPtr,numWeCanWrite);

        readPtr += numWeCanWrite;
        numInBuffer += numWeCanWrite;
        stillToWrite -= numWeCanWrite;

        if (numInBuffer == maxBufferSize)
          flush();
      }
    }

    void BufferedWriteStream::flush()
    {
      if (numInBuffer > 0)
        fabric.get().send(buffer,numInBuffer);
      numInBuffer = 0;
    }

  } // ::ospcommon::networking
} // ::ospcommon
