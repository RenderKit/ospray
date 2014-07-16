/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/common/ospcommon.h"
#include "ospray/api/handle.h"

namespace ospray {
  namespace coi {
    using std::cout; 
    using std::cerr; 
    using std::endl;
    using std::flush;

    using ospray::api::Handle;

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
  }
}
