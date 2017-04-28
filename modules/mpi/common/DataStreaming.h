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

#pragma once

#include "mpiCommon/MPICommon.h"

namespace ospray {
  namespace mpi {

    /*! abstraction of an object that we can serailize/write (raw) data into */
    struct WriteStream
    {
      virtual void write(void *mem, size_t size) = 0;
      virtual void flush() {}
    };
      
    /*! abstraction of an object that we can read (raw) data from to
      then de-serialize into work objects */
    struct ReadStream
    {
      virtual void read(void *mem, size_t size) = 0;
    };


    /*! generic stream operators into/out of read/write streams, for raw data blocks */
    template<typename T>
    inline WriteStream &operator<<(WriteStream &buf, const T &rh)
    {
      buf.write((mpicommon::byte_t*)&rh, sizeof(T));
      return buf;
    }

    template<typename T>
    inline ReadStream &operator>>(ReadStream &buf, T &rh)
    {
      buf.read((mpicommon::byte_t*)&rh, sizeof(T));
      return buf;
    }

    /*! @{ stream operators into/out of read/write streams, for std::vectors of data 

      \warning This code is often _expensive_ - if this, is for
      example, a std::vector<byte>, then we'll do a buf.write for each
      and every one of these vector entries. for cases like the, what
      the caller _should_ do is to manually write first the size, and
      then _all_ items in a single block. Still, it is good to have
      this in here for correctness, so I'll leave it in here - the
      alternative would be to explciitly 'throw' an execptoin in there
      to prevent the user from using this slow-path in the first place.
      Finally - last comment on this - we might actually want to do add
      some templates
     */
    template<typename T>
    inline WriteStream &operator<<(WriteStream &buf, const std::vector<T> &rh)
    {
      buf << rh.size();
      for (const T &v : rh)
        buf << v;
      return buf;
    }

    template<typename T>
    inline ReadStream &operator>>(ReadStream &buf, std::vector<T> &rh)
    {
      size_t sz;
      buf >> sz;
      rh.resize(sz);
      for (T &v : rh)
        buf >> v;
      return buf;
    }
    /*! @} */

    
      
    /*! @{ serialize operations for strings */
    inline WriteStream &operator<<(WriteStream &buf, const std::string &rh)
    {
      buf << rh.size();
      buf.write((mpicommon::byte_t*)rh.c_str(), rh.size());
      return buf;
    }

    inline ReadStream &operator>>(ReadStream &buf, std::string &rh)
    {
      size_t size;
      buf >> size;
      rh = std::string(size, ' ');
      buf.read((mpicommon::byte_t*)rh.data(), size);
      return buf;
    }
    /*! @} */

  } // ::ospray::mpi
} // ::ospray

      
