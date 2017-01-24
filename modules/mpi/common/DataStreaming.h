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

namespace ospray {
  namespace mpi {

    /*! abstraction of an object that we can serailize/write (raw) data into */
    struct WriteStream {
      virtual void write(void *mem, size_t size) = 0;
      virtual void flush() {};
    };
      
    /*! abstraction of an object that we can read (raw) data from to
      then de-serialize into work objects */
    struct ReadStream {
      virtual void read(void *mem, size_t size) = 0;
    };


    /*! @{ gneric stream operators into/out of read/write streams, for raw data blocks */
    template<typename T>
    inline WriteStream &operator<<(WriteStream &buf, const T &rh)
    {
      buf.write((byte_t*)&rh, sizeof(T));
      return buf;
    }

    template<typename T>
    inline ReadStream &operator>>(ReadStream &buf, T &rh)
    {
      buf.read((byte_t*)&rh, sizeof(T));
      return buf;
    }
    /*! @} */

      
    /*! @{ serialize operations for strings */
    inline WriteStream &operator<<(WriteStream &buf, const std::string &rh)
    {
      buf << rh.size();
      buf.write((byte_t*)rh.c_str(), rh.size());
      return buf;
    }

    inline ReadStream &operator>>(ReadStream &buf, std::string &rh)
    {
      size_t size;
      buf >> size;
      rh = std::string(size, ' ');
      buf.read((byte_t*)&rh[0], size);
      return buf;
    }
    /*! @} */

  } // ::ospray::mpi
} // ::ospray

      
