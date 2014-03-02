#include "mpi/mpicommon.h"
#include "common/ospcommon.h"

namespace ospray {

  extern "C" void ospray_init_module_MPItest() 
  {
    printf("#mpi test module loaded...\n");
  }
}


