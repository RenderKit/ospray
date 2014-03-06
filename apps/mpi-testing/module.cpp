#include "mpi/mpicommon.h"
#include "common/ospcommon.h"

namespace ospray {
  void bwTestMaster()
  {
    MPI_Status status;
    int msgSize;
    while (1) {
      MPI_Recv(&msgSize,1,MPI_INT,0,0,mpi::app.comm,&status);
      if (msgSize == -1)
        // done testing
        return;
      while (1) {
        char data[msgSize];
        // receeive data block
        MPI_Recv(data,msgSize,MPI_CHAR,0,0,mpi::app.comm,&status);
        if (data[0] == 1)
          break;
        // and send it right back
        MPI_Send(data,msgSize,MPI_CHAR,0,0,mpi::app.comm);
      }
    }
  }

  extern "C" void ospray_init_module_MPItest() 
  {
    printf("#mpi test module loaded...\n");
    if (mpi::app.rank == 0) /* this is the master ... */ return;
    bwTestMaster();
  }
}


