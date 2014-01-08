#include "mpicommon.h"

namespace ospray {
  namespace MPI {
    
    int rank = -1;
    int size = -1;

    MPI_Comm serviceComm = MPI_COMM_NULL;

    void init(int *ac, const char **av)
    {
      MPI_Init(ac,(char ***)&av);
      MPI_Comm_rank(MPI_COMM_WORLD,&rank);
      MPI_Comm_size(MPI_COMM_WORLD,&size);
    }
  }

}
