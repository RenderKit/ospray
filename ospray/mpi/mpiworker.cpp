#include "../mpi/mpicommon.h"
#include "../mpi/mpidevice.h"

namespace ospray {
  namespace mpi {
  using std::cout;
  using std::endl;

  void runWorker(int *_ac, const char **_av);

  void workerMain(int ac, const char **av)
  {
    int rc;
    mpi::init(&ac,av);
    worker.comm = world.comm;
    worker.makeIntercomm();

    char servicePortName[MPI_MAX_PORT_NAME];
    if (world.rank == 0) {
      rc = MPI_Open_port(MPI_INFO_NULL, servicePortName);
      Assert(rc == MPI_SUCCESS);
      cout << "------------------------------------------------------" << endl;
      cout << "ospray service started with " << worker.size << " workers" << endl;
      cout << "OSPRAY_SERVICE_PORT:" << servicePortName << endl;
      rc = MPI_Comm_accept(servicePortName,MPI_INFO_NULL,0,MPI_COMM_WORLD,&app.comm);
      Assert(rc == MPI_SUCCESS);
      app.makeIntracomm();
    }
    MPI_Barrier(world.comm);
    runWorker(&ac,av); // this fct will not return
  }
  }
}

int main(int ac, const char **av)
{
  ospray::mpi::workerMain(ac,av);
}
