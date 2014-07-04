#include "common/ospcommon.h"
#include "../api/handle.h"

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
      // void write(const int32 &ID) {
      //   write(&ID,sizeof(ID));
      // }
      // void write(const int64 &ID) {
      //   write(&ID,sizeof(ID));
      // }
      // void write(const uint64 &ID) {
      //   write(&ID,sizeof(ID));
      // }
      template<typename T>
      inline void write(const T &t) {
        write(&t,sizeof(t));
      }
      void write(const char *s) {
        int32 len = strlen(s);
        write(s,len+1);
      }
      // void write(const Handle &handle) {
      //   write(&handle,sizeof(handle));
      // }

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

      // inline int32 getInt() {
      //   int32 h;
      //   memcpy(&h,buf+ofs,sizeof(h));
      //   ofs += sizeof(h);
      //   return h;
      // }

      // inline Handle getHandle() {
      //   Handle h;
      //   memcpy(&h,buf+ofs,sizeof(h));
      //   ofs += sizeof(h);
      //   return h;
      // }
    };
  }
}
