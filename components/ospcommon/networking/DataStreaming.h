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

#include "../common.h"
#include "../utility/ArrayView.h"
#include "../TypeTraits.h"

#include <vector>

namespace ospcommon {
  namespace networking {

    /*! abstraction of an object that we can serailize/write (raw) data into */
    struct WriteStream
    {
      virtual ~WriteStream() = default;

      virtual void write(const void *mem, size_t size) = 0;
      virtual void flush() {}
    };

    /*! abstraction of an object that we can read (raw) data from to
      then de-serialize into work objects */
    struct ReadStream
    {
      virtual ~ReadStream() = default;

      virtual void read(void *mem, size_t size) = 0;
    };

    /*! generic stream operators into/out of streams, for raw data blocks */
    template<typename T>
    inline WriteStream &operator<<(WriteStream &buf, const T &rh)
    {
      buf.write((const byte_t*)&rh, sizeof(T));
      return buf;
    }

    template<typename T>
    inline ReadStream &operator>>(ReadStream &buf, T &rh)
    {
      buf.read((byte_t*)&rh, sizeof(T));
      return buf;
    }

    /*! @{ stream operators into/out of read/write streams, for std::vectors
     * of non-POD types*/
    template<typename T>
    inline WriteStream &operator<<(WriteStream &buf, const std::vector<T> &rh)
    {
      const size_t sz = rh.size();
      buf << sz;

      for (const auto &x : rh)
        buf << x;

      return buf;
    }

    template<typename T>
    inline ReadStream &operator>>(ReadStream &buf, std::vector<T> &rh)
    {
      size_t sz;
      buf >> sz;
      rh.resize(sz);

      for (size_t i = 0; i < sz; ++i)
        buf >> rh[i];

      return buf;
    }
    /*! @} */

    /*! @{ stream operators into/out of read/write streams, for ArrayView<T> */
    template<typename T>
    inline WriteStream &operator<<(WriteStream &buf,
                                   const utility::ArrayView<T> &rh)
    {
      const size_t sz = rh.size();
      buf << sz;
      buf.write((const byte_t*)rh.data(), sizeof(T)*sz);
      return buf;
    }
    /*! @} */

    /*! @{ serialize operations for strings */
    inline WriteStream &operator<<(WriteStream &buf, const std::string &rh)
    {
      const size_t sz = rh.size();
      buf << sz;
      buf.write((const void *)rh.data(), sz);
      return buf;
    }

    inline ReadStream &operator>>(ReadStream &buf, std::string &rh)
    {
      size_t sz;
      buf >> sz;
      rh.resize(sz);
      buf.read((void *)rh.data(), sz);
      return buf;
    }
    /*! @} */

  } // ::ospcommon::networking
} // ::ospcommon
