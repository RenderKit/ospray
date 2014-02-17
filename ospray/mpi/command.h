#pragma once

#include "mpicommon.h"
#include "handle.h"

namespace ospray {
  namespace mpi {
    /*! \brief abstraction for a binary command stream */
    /*! abstracts the concept of the mpi device writing commands and
      parameters, and the respective worker reading and unpacking
      those parameters, into a wrapper class. allows for implementing
      the actual communication via different methods (MPI, sockets,
      COI) as well as tweaking the implementation in a way that will
      apply equally to all functions */
    struct CommandStream {
      void newCommand(int tag) {
        int rc = MPI_Bcast(&tag,1,MPI_INT,MPI_ROOT,mpi::worker.comm); 
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(const void *data, const size_t size)
      {
        Assert(data);
        int rc = MPI_Bcast((void*)data,size,MPI_BYTE,MPI_ROOT,mpi::worker.comm);
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(int32 i)
      { 
        int rc = MPI_Bcast(&i,1,MPI_INT,MPI_ROOT,mpi::worker.comm);
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(size_t i)
      { 
        int rc = MPI_Bcast(&i,1,MPI_LONG,MPI_ROOT,mpi::worker.comm);
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(const vec2i &v)
      { 
        int rc = MPI_Bcast((void*)&v,2,MPI_INT,MPI_ROOT,mpi::worker.comm);
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(const vec3f &v)
      { 
        int rc = MPI_Bcast((void*)&v,3,MPI_FLOAT,MPI_ROOT,mpi::worker.comm);
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(const vec3i &v)
      { 
        int rc = MPI_Bcast((void*)&v,3,MPI_INT,MPI_ROOT,mpi::worker.comm);
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(uint32 i)
      { 
        int rc = MPI_Bcast(&i,1,MPI_INT,MPI_ROOT,mpi::worker.comm);       
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(const Handle &h)
      { 
        int rc = MPI_Bcast((void*)&h,2,MPI_INT,MPI_ROOT,mpi::worker.comm);
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(float f)
      { 
        int rc = MPI_Bcast(&f,1,MPI_FLOAT,MPI_ROOT,mpi::worker.comm); 
        Assert(rc == MPI_SUCCESS); 
      }
      inline void send(const char *s)
      { 
        int len = strlen(s);
        send(len);
        int rc = MPI_Bcast((void*)s,len,MPI_CHAR,MPI_ROOT,mpi::worker.comm); 
        Assert(rc == MPI_SUCCESS); 
      }

      inline int get_int32() 
      { 
        int v; 
        int rc = MPI_Bcast(&v,1,MPI_INT,0,mpi::app.comm); 
        Assert(rc == MPI_SUCCESS); 
        return v; 
      }
      inline size_t get_size_t() 
      { 
        int64 v; 
        int rc = MPI_Bcast(&v,1,MPI_LONG,0,mpi::app.comm); 
        Assert(rc == MPI_SUCCESS); 
        return v; 
      }
      inline size_t get_data(void *&pointer) 
      { 
        int64 size; 
        int rc = MPI_Bcast(&size,1,MPI_LONG,0,mpi::app.comm); 
        Assert(rc == MPI_SUCCESS); 
        if (size == 0) pointer = NULL; 
        else {
          pointer = malloc(size);
          int rc = MPI_Bcast(pointer,size,MPI_BYTE,0,mpi::app.comm); 
          Assert(rc == MPI_SUCCESS); 
        }
        return size; 
      }
      inline Handle get_handle() 
      { 
        Handle v; 
        int rc = MPI_Bcast(&v,2,MPI_INT,0,mpi::app.comm); 
        Assert(rc == MPI_SUCCESS); 
        return v; 
      }
      inline vec2i get_vec2i() 
      { 
        vec2i v; 
        int rc = MPI_Bcast(&v,2,MPI_INT,0,mpi::app.comm); 
        Assert(rc == MPI_SUCCESS); 
        return v; 
      }
      inline vec3f get_vec3f() 
      { 
        vec3f v; 
        int rc = MPI_Bcast(&v,3,MPI_FLOAT,0,mpi::app.comm); 
        Assert(rc == MPI_SUCCESS); 
        return v; 
      }
      inline vec3i get_vec3i() 
      { 
        vec3i v; 
        int rc = MPI_Bcast(&v,3,MPI_INT,0,mpi::app.comm); 
        Assert(rc == MPI_SUCCESS); 
        return v; 
      }
      inline float get_float() 
      { 
        float v; 
        int rc = MPI_Bcast(&v,1,MPI_FLOAT,0,mpi::app.comm); 
        Assert(rc == MPI_SUCCESS); 
        return v; 
      }
      inline void free(const char *s) 
      { Assert(s); ::free((void*)s); }
      inline const char *get_charPtr() 
      { 
        int len = get_int32();
        char buf[len+1]; buf[len] = 0;
        int rc = MPI_Bcast(buf,len,MPI_CHAR,0,mpi::app.comm); 
        Assert(rc == MPI_SUCCESS); 
        return strdup(buf); 
      }
      void flush() {};
    };
  } 
}
