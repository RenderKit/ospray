#pragma once

#include <mpi.h>
#include "../common/ospcommon.h"

namespace ospray {
  
  /*! Helper class for MPI Programming */
  namespace MPI {
    extern int rank;
    extern int size;

    extern MPI_Comm workerLink; /*! contains all worker clients. this allows
                             the master to talk to the client(s), and
                             the clients to talk amongst each other */
    extern MPI_Comm masterLink; /*! on any worker client, links back to the server node */
    void init(int *ac, const char **av);
  };

}
