#include "mpicommon.h"

namespace ospray {
  namespace MPI {
    
    void init(int *ac, const char **av)
    {
      PING;
      MPI_Init(ac,(char ***)&av);
      PING;
    }
  }

}
