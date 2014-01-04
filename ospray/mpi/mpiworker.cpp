#include "../mpi/mpicommon.h"

namespace ospray {
  void MPIworker(int ac, const char **av)
  {
    PING;
    MPI::init(&ac,av);
    PING;
  }
}

int main(int ac, const char **av)
{
  ospray::MPIworker(ac,av);
}
