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

#include "common/OSPCommon.h"
#include "common/ObjectHandle.h"

namespace ospray {
  namespace coi {

    using std::cout; 
    using std::cerr; 
    using std::endl;
    using std::flush;

    struct DataStream {
      char *buf;
      size_t size;
      size_t ofs;

      DataStream() 
      {
        size = 1024;
        buf = new char[size];
        ofs = 0;
      }
      DataStream(const void *buf) 
        : buf((char*)buf), ofs(0), size(0)
      {
      }
      ~DataStream() {
        if (size) delete[] buf;
      }

      inline void write(const void *mem, size_t bytes) 
      { 
        if (ofs+bytes >= size) {
          while (ofs+bytes >= size) size *= 2;
          buf = (char*)realloc(buf,size);
        }
        memcpy(buf+ofs,mem,bytes);
        ofs+=bytes;
      }

      template<typename T>
      inline void write(const T &t) {
        write(&t,sizeof(t));
      }
      void write(const char *s) {
        int32 len = strlen(s);
        write(s,len+1);
      }

      template<typename T>
      inline T get() {
        T h;
        memcpy(&h,buf+ofs,sizeof(h));
        ofs += sizeof(h);
        return h;
      }

      inline const char *getString() {
        const char *s = buf+ofs;
        ofs += (strlen(s)+1);
        return s;
      }
    };

  } // ::ospray::coi
} // ::ospray
