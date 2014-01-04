#include "mpicommon.h"

namespace ospray {

  void MPI::init(int *ac, const char **av)
  {
    MPI_Init(ac,av);
  }
}
