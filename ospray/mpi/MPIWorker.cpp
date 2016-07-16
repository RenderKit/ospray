// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "mpi/MPICommon.h"
#include "mpi/MPIDevice.h"

#include "ospray/ospray.h"

namespace ospray {
  namespace mpi {

    using std::cout;
    using std::endl;

    OSPRAY_INTERFACE void runWorker();

    void workerMain(int ac, const char **av)
    {
      if (ac < 2) {
        int argc = 2;
        const char *argv[] = {"ospray_mpi_worker", "--osp:mpi"};
        ospInit(&argc, argv);
      } else {
        ospInit(&ac, av);
      }

      int rc;
      mpi::init(&ac,av);
      worker.comm = world.comm;
      worker.makeIntraComm();

      if (ac == 3 && !strcmp(av[1],"--osp:connect")) {
        char *appPortName = strdup(av[2]);
        // de-fix port name - 'real' port name has '$'s, not '%'s
        for (char *s = appPortName; *s; ++s)
          if (*s == '%') *s = '$';
        cout << "#w: trying to connect to port " << appPortName << endl;
        rc = MPI_Comm_connect(appPortName,MPI_INFO_NULL,0,
                              worker.comm,&app.comm);
        Assert(rc == MPI_SUCCESS);
        cout << "#w: ospray started with " << worker.size << " workers, "
             << "#w: and connected to app at  " << appPortName << endl;
        app.makeInterComm();
      } else {
        char servicePortName[MPI_MAX_PORT_NAME];
        if (world.rank == 0) {
          rc = MPI_Open_port(MPI_INFO_NULL, servicePortName);
          cout << "#osp:ospray_mpi_worker opened port: " << servicePortName
               << endl;
          for (char *s = servicePortName; *s ; s++)
            if (*s == '$') *s = '%';

          Assert(rc == MPI_SUCCESS);
          cout << "------------------------------------------------------"
               << endl;
          cout << "ospray service started with " << worker.size
               << " workers" << endl;
          cout << "OSPRAY_SERVICE_PORT:" << servicePortName << endl;
        }
        cout << "#osp:ospray_mpi_worker trying to accept from '"
             << servicePortName << "'" << endl;
        rc = MPI_Comm_accept(servicePortName,MPI_INFO_NULL,
                             0,MPI_COMM_WORLD,&app.comm);
        Assert(rc == MPI_SUCCESS);
        app.makeInterComm();
      }
      MPI_Barrier(world.comm);
      runWorker(); // this fct will not return
    }

  } // ::ospray::api
} // ::ospray

int main(int ac, const char **av)
{
  ospray::mpi::workerMain(ac,av);
}
