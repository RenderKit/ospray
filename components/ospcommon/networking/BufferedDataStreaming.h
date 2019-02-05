// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "DataStreaming.h"
#include "Fabric.h"

namespace ospcommon {
  namespace networking {

    /* read stream that serves smaller read requests (of a given
       size) from a block of data that it queries from a fabric. if
       the internal buffer isn't big enough to fulfill the request,
       the next block will automatically get read from the fabric */
    struct OSPCOMMON_INTERFACE BufferedReadStream : public ReadStream
    {
      BufferedReadStream(Fabric &fabric);
      ~BufferedReadStream() override = default;

      void read(void *mem, size_t size) override;

    private:

      std::reference_wrapper<Fabric> fabric;
      ospcommon::byte_t *buffer;
      size_t numAvailable;
    };

    /*! maintains an internal buffer of a given size, and buffers
      all write ops preferably into this buffer; this internal
      buffer gets flushed either when the user explicitly calls
      flush(), or when the maximum size of the buffer gets
      reached */
    struct OSPCOMMON_INTERFACE BufferedWriteStream : public WriteStream
    {
      BufferedWriteStream(Fabric &fabric, size_t maxBufferSize = 1LL*1024*1024);
      ~BufferedWriteStream() override;

      void write(const void *mem, size_t size) override;
      void flush() override;

    private:

      std::reference_wrapper<Fabric> fabric;
      ospcommon::byte_t *buffer;
      size_t  maxBufferSize;
      size_t  numInBuffer;
    };

  } // ::ospcommon::networking
} // ::ospcommon
