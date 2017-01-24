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

#include "DataStreaming.h"

namespace ospray {
  namespace mpi {

    /*! abstraction for a physical fabric that can transmit data -
      sockets, mpi, etc */
    struct Fabric {
      /*! send exact number of bytes - the fabric can do that through
        multiple smaller messages, but all bytes have to be
        delivered */
      virtual void   send(void *mem, size_t s) = 0;

      /*! receive some block of data - whatever the sender has sent -
        and give us size and pointer to this data */
      virtual size_t recv(void *&mem) = 0;
    };


    /*! a specific fabric based on PMI */
    struct MPIBcastFabric : public Fabric {
      MPIBcastFabric(const mpi::Group &group)
        : group(group),
          buffer(nullptr)
      {}
      
      /*! send exact number of bytes - the fabric can do that through
        multiple smaller messages, but all bytes have to be
        delivered */
      virtual void   send(void *mem, size_t size) override
      {
        assert(size < (1LL<<30));
        uint32_t sz32 = size;
        MPI_CALL(Bcast(&sz32,1,MPI_INT,MPI_ROOT,group.comm));
        MPI_CALL(Bcast(mem,sz32,MPI_BYTE,MPI_ROOT,group.comm));
      }

      /*! receive some block of data - whatever the sender has sent -
        and give us size and pointer to this data */
      virtual size_t recv(void *&mem) override
      {
        if (buffer) delete[] buffer;

        uint32_t sz32 = 0;
        MPI_CALL(Bcast(&sz32,1,MPI_INT,0,group.comm));
        buffer = new uint8_t[sz32];
        MPI_CALL(Bcast(buffer,sz32,MPI_BYTE,0,group.comm));
        mem = buffer;
        return sz32;
      }
      
      uint8_t *buffer;
      mpi::Group group;
    };


    /*! contains implementations for buffered read and write streams,
        that fulfill smaller read/write requests from a large buffer
        that thos classes maintain */
    struct BufferedFabric {

      /* read stream that serves smaller read requests (of a given
         siez) from a block of data that it queries from a fabric. if
         the internal buffer isn't big enough to fulfill the request,
         te next block will automatically get read from the fabric */ 
      struct ReadStream : public ospray::mpi::ReadStream {
        ReadStream(std::shared_ptr<Fabric> fabric)
          : fabric(fabric)
        {}
        
        virtual void read(void *mem, size_t size) override;

        std::shared_ptr<Fabric> fabric;
        uint8_t *buffer;
        size_t   numAvailable;
      };

      /*! maintains an internal buffer of a given size, and buffers
        asll write ops preferably into this buffer; this internal
        buffer gets flushed either when the user explcitly calls
        flush(), or when the maximum size of the buffer gets
        reached */
      struct WriteStream : public ospray::mpi::WriteStream {
        WriteStream(std::shared_ptr<Fabric> fabric, size_t maxBufferSize = 1LL*1024*1024)
          : fabric(fabric),
            maxBufferSize(maxBufferSize),
            numInBuffer(0),
            buffer(new uint8_t[maxBufferSize])
        {}
        ~WriteStream()
        { delete buffer; }
        
        virtual void write(void *mem, size_t size) override;
        
        virtual void flush() override;
          
        std::shared_ptr<Fabric> fabric;
        uint8_t *buffer;
        size_t maxBufferSize;
        size_t numInBuffer;
      };
      
    };
    
  } // ::ospray::mpi
} // ::ospray
