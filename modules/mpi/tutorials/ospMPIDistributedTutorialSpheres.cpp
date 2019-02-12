// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
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

#include <iterator>
#include <memory>
#include <random>
#include <mpi.h>
#include "GLFWDistribOSPRayWindow.h"

#include "ospcommon/library.h"
#include "ospray_testing.h"

#include <imgui.h>

using namespace ospcommon;

int main(int argc, char **argv)
{
  int mpiThreadCapability = 0;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpiThreadCapability);
  if (mpiThreadCapability != MPI_THREAD_MULTIPLE
      && mpiThreadCapability != MPI_THREAD_SERIALIZED)
  {
    fprintf(stderr, "OSPRay requires the MPI runtime to support thread "
            "multiple or thread serialized.\n");
    return 1;
  }

  int mpiRank = 0;
  int mpiWorldSize = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiWorldSize);

  // load the MPI module, and select the MPI distributed device. Here we
  // do not call ospInit, as we want to explicitly pick the distributed
  // device. This can also be done by passing --osp:mpi-distributed when
  // using ospInit, however if the user doesn't pass this argument your
  // application will likely not behave as expected
  ospLoadModule("mpi");

  OSPDevice mpiDevice = ospNewDevice("mpi_distributed");
  ospDeviceCommit(mpiDevice);
  ospSetCurrentDevice(mpiDevice);

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create the "world" model which will contain all of our geometries
  OSPModel world = ospNewModel();

  // all ranks specify the same rendering parameters, with the exception of
  // the data to be rendered, which is distributed among the ranks
  // triangle mesh data
  float vertex[] = { mpiRank, 0.0f, 3.5f, 0.f,
                     mpiRank, 1.0f, 3.0f, 0.f,
                     1.0f * (mpiRank + 1.f), 0.0f, 3.0f, 0.f,
                     1.0f * (mpiRank + 1.f), 1.0f, 2.5f, 0.f };
  float color[] =  { 0.0f, 0.0f, (mpiRank + 1.f) / mpiWorldSize, 1.0f,
                     0.0f, 0.0f, (mpiRank + 1.f) / mpiWorldSize, 1.0f,
                     0.0f, 0.0f, (mpiRank + 1.f) / mpiWorldSize, 1.0f,
                     0.0f, 0.0f, (mpiRank + 1.f) / mpiWorldSize, 1.0f };
  int32_t index[] = { 0, 1, 2,
                      1, 2, 3 };

  // create and setup model and mesh
  OSPGeometry mesh = ospNewGeometry("triangles");
  OSPData data = ospNewData(4, OSP_FLOAT3A, vertex, 0); // OSP_FLOAT3 format is also supported for vertex positions
  ospCommit(data);
  ospSetData(mesh, "vertex", data);
  ospRelease(data); // we are done using this handle

  data = ospNewData(4, OSP_FLOAT4, color, 0);
  ospCommit(data);
  ospSetData(mesh, "vertex.color", data);
  ospRelease(data); // we are done using this handle

  data = ospNewData(2, OSP_INT3, index, 0); // OSP_INT4 format is also supported for triangle indices
  ospCommit(data);
  ospSetData(mesh, "index", data);
  ospRelease(data); // we are done using this handle

  ospCommit(mesh);
  ospAddGeometry(world, mesh);

  // add in spheres geometry
  //OSPTestingGeometry spheres = ospTestingNewGeometry("spheres", "pathtracer");
  //ospAddGeometry(world, spheres);
  //ospRelease(spheres);

  ospSet1i(world, "id", mpiRank);
  // commit the world model
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("mpi_raycast");

  // create and setup an ambient light
  OSPLight light = ospNewLight3("ambient");
  ospCommit(light);
  OSPData lights = ospNewData(1, OSP_LIGHT, &light, 0);
  ospCommit(lights);
  ospSetObject(renderer, "lights", lights);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWDistribOSPRayWindow>(new GLFWDistribOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

  int spp = 1;
  int currentSpp = 1;
  if (mpiRank == 0) {
    glfwOSPRayWindow->registerImGuiCallback([&]() {
      ImGui::SliderInt("spp", &spp, 1, 64);
    });
  }

  glfwOSPRayWindow->registerDisplayCallback([&](GLFWDistribOSPRayWindow *win) {
    // Send the UI changes out to the other ranks so we can synchronize
    // how many samples per-pixel we're taking
    MPI_Bcast(&spp, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (spp != currentSpp) {
      currentSpp = spp;
      ospSet1i(renderer, "spp", spp);
      win->addObjectToCommit(renderer);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(world);
  ospRelease(renderer);

  // cleanly shut OSPRay down
  ospShutdown();

  MPI_Finalize();

  return 0;
}
