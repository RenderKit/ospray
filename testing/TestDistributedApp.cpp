// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include <mpi.h>
#include <ospray/ospray.h>

#define Assert(cond, err) \
  if (!(cond))            \
    throw std::runtime_error(err);

namespace ospray {

  /*! global camera, handle is valid across all ranks */
  OSPCamera camera = NULL;
  /*! global renderer, handle is valid across all ranks */
  OSPRenderer renderer = NULL;
  /*! global frame buffer, handle is valid across all ranks */
  OSPFrameBuffer fb = NULL;
  /*! global model, handle is valid across all ranks */
  OSPWorld model = NULL;

  void testDistributedApp_main(int &ac, char **&av)
  {
    ospdMpiInit(&ac, &av, OSPD_Z_COMPOSITE);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    printf("#ospdapp: starting test app for osp distributed mode, rank %i/%i\n",
           rank,
           size);
    MPI_Barrier(MPI_COMM_WORLD);

    ospdApiMode(OSPD_ALL);
    model = ospNewWorld();
    Assert(model, "error creating model");

    camera = ospNewCamera("perspective");
    Assert(camera, "error creating camera");

    renderer = ospNewRenderer("obj");
    Assert(renderer, "error creating renderer");

    ospdApiMode(OSPD_RANK);

    MPI_Barrier(MPI_COMM_WORLD);
    printf("---------------------- SHUTTING DOWN ----------------------\n");
    fflush(0);
    MPI_Barrier(MPI_COMM_WORLD);
    ospdMpiShutdown();
  }
}  // namespace ospray

int main(int ac, char **av)
{
  ospray::testDistributedApp_main(ac, av);
  return 0;
}
