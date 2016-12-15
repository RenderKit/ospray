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

/*! \file ospray/device/nwlayer.h \brief Implement utility read/write
    buffers that can be used to send data across all kinds of
    network/communication devices */

#include "common/OSPCommon.h"

namespace ospray {
  namespace nwlayer {

    /*! implements a local data buffer that keeps data sent through a
        network device, and that the reader can read typed data
        from */
    struct ReadBuffer {
      unsigned char *mem;
      size_t         size;
      size_t         next; /*!< next posiiton we're reading from */
      bool           mine;
      inline ReadBuffer() 
        : mem(NULL), size(0), mine(false) 
      {}
      inline ReadBuffer(size_t sz) 
        : mem((unsigned char*)malloc(sz)), size(sz), next(0), mine(true)
      { assert(mem); }
      inline ReadBuffer(unsigned char *ptr, size_t sz) 
        : mem(ptr), size(sz), next(0), mine(false)
      {}
      inline ~ReadBuffer() 
      { if (mine) free(mem); }
      inline void read(void *t, size_t sz) 
      { assert(next+sz <= size); memcpy(t,mem+next,sz); next+=sz; }

      /*! helper function to read typed data from a recv buffer */
      template<typename T> inline T read() 
      { T t; this->read(&t,sizeof(t)); return t; }
    };
    
    /*! implements a local data buffer that one can write data to;
        this data then gets queues up in a single memory region, from
        where it can be sent in a single network message */
    struct WriteBuffer {
      static const int initialReservedSize = 16*1024;
      unsigned char *mem;
      size_t         size;
      size_t         reserved;
      /*! constructor allocate new empty data stream of initial
          reserved size */
      inline WriteBuffer() 
        : mem((unsigned char *)malloc(initialReservedSize)), 
          size(0), 
          reserved(initialReservedSize)
      { assert(mem); };
      /*! destructor - free alloc'ed memory */
      inline ~WriteBuffer()
      { free((char*)mem); }
      /*! reserve at least 'delta' bytes at end of stream */
      inline void reserve(size_t delta) {
        if (size+delta <= reserved) return;
        while (size+delta > reserved) reserved *= 2;
        mem = (unsigned char *)realloc(mem,reserved);
      }
      /*! append new (untyped) block of mem to buffer */
      inline void write(const void *t, size_t t_size) 
      { reserve(t_size); memcpy(mem+size,t,t_size); size+=t_size; }

      /*! helper function to write typed data into this buffer */
      template<typename T> inline void write(const T &t) 
      { this->write(&t,sizeof(t)); }
    };

    /*! explicit instantiation for string types */
    template<> inline const char *ReadBuffer::read<const char *>()
    { 
      int32 len = read<int32>(); 
      char *s = (char*)malloc(len+1); 
      read(s,len+1);
      return s;
    }
    /*! explicit instantiation for string types */
    template<> inline char *ReadBuffer::read<char *>() 
    { 
      return (char*)read<const char *>(); 
    }
    /*! explicit instantiation for string types */
    template<> inline std::string ReadBuffer::read<std::string>() 
    {      
      const char *s = read<const char *>(); 
      std::string ss = s;
      free((char*)s);
      return s;
    }


    /*! explicit instantiation for string types */
    template<> inline void WriteBuffer::write<const char *>(const char *const &s) 
    {
      const int32 len = strlen(s); 
      write(len); 
      this->write(s,len+1); 
    }
    /*! explicit instantiation for string types */
    template<> inline void WriteBuffer::write(const std::string &s) {
      write(s.c_str());
    }

  } // ::ospray::nwlayer
} // ::ospray
