/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

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

    if (ac == 3 && !strcmp(av[1],"--osp:connect")) {
      // if (worker.rank == 0) {
      char *appPortName = strdup(av[2]);
      // de-fix port name - 'real' port name has '$'s, not '%'s
      for (char *s = appPortName; *s; ++s)
        if (*s == '%') *s = '$';
      cout << "#w: trying to connect to port " << appPortName << endl;
      rc = MPI_Comm_connect(appPortName,MPI_INFO_NULL,0,worker.comm,&app.comm);
      Assert(rc == MPI_SUCCESS);
      cout << "#w: ospray started with " << worker.size << " workers, "
           << "#w: and connected to app at  " << appPortName << endl;
      // }
      app.makeIntracomm();
    } else {
      char servicePortName[MPI_MAX_PORT_NAME];
      if (world.rank == 0) {
        rc = MPI_Open_port(MPI_INFO_NULL, servicePortName);
        for (char *s = servicePortName;*s;s++)
          if (*s == '$') *s = '%';
        Assert(rc == MPI_SUCCESS);
        cout << "------------------------------------------------------" << endl;
        cout << "ospray service started with " << worker.size << " workers" << endl;
        cout << "OSPRAY_SERVICE_PORT:" << servicePortName << endl;
      }
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
