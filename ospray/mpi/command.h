#pragma once

#include "mpicommon.h"

namespace ospray {

  /*! \brief abstraction for a binary command stream */
  /*! abstracts the concept of the mpi device writing commands and
    parameters, and the respective worker reading and unpacking
    those parameters, into a wrapper class. allows for implementing
    the actual communication via different methods (MPI, sockets,
    COI) as well as tweaking the implementation in a way that will
    apply equally to all functions */
  struct MPICommandStream {
    void newCommand(int tag) {
      int rc = MPI_Bcast(&tag,1,MPI_INT,MPI_ROOT,mpi::worker.comm); 
      Assert(rc == MPI_SUCCESS); 
    }
    inline void send(int32 i)
    { 
      int rc = MPI_Bcast(&i,1,MPI_INT,MPI_ROOT,mpi::worker.comm);
      Assert(rc == MPI_SUCCESS); 
    }
    inline void send(const vec2i &v)
    { 
      int rc = MPI_Bcast((void*)&v,2,MPI_INT,MPI_ROOT,mpi::worker.comm);
      Assert(rc == MPI_SUCCESS); 
    }
    inline void send(uint32 i)
    { 
      int rc = MPI_Bcast(&i,1,MPI_INT,MPI_ROOT,mpi::worker.comm);       
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
    inline vec2i get_vec2i() 
    { 
      vec2i v; 
      int rc = MPI_Bcast(&v,2,MPI_INT,0,mpi::app.comm); 
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
