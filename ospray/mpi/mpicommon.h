#pragma once

#include <mpi.h>
#include "../common/ospcommon.h"

namespace ospray {
  
  /*! Helper class for MPI Programming */
  struct MPI {
    static int rank;
    static int size;

    void init(int *ac, const char **av);
  };

}
