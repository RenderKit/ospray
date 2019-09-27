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

/* This is a small example tutorial how to use OSPRay and the
 * MPIDistributedDevice in a data-parallel application.
 * Each rank must specify the same render parameters, however the data
 * to render on each rank can differ for distributed rendering. In this
 * tutorial each rank renders a unique quad, which is colored by the rank.
 *
 * On Linux build it in the build_directory with:
 *   mpicxx ../modules/mpi/tutorials/ospMPIDistributedTutorial.cpp \
 *       -I ../ospray/include -I ../components \
 *       -L . -lospray -lospray_common -Wl,-rpath,. \
 *       -o ospMPIDistributedTutorialCpp
 *
 * Then run it with MPI on some number of processes
 *   mpirun -n <N> ./ospMPIDistributedTutorialCpp
 *
 * The output image should show a sequence of quads, from dark to light blue
 */

#include <errno.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#define NOMINMAX
#include <malloc.h>
#else
#include <alloca.h>
#endif

#include "ospray/ospray_cpp.h"

using namespace ospcommon;
using namespace math;

// helper function to write the rendered image as PPM file
void writePPM(const char *fileName, const vec2i &size, const uint32_t *pixel)
{
  FILE *file = fopen(fileName, "wb");
  if (file == nullptr) {
    fprintf(stderr, "fopen('%s', 'wb') failed: %d", fileName, errno);
    return;
  }
  fprintf(file, "P6\n%i %i\n255\n", size.x, size.y);
  unsigned char *out = (unsigned char *)alloca(3 * size.x);
  for (int y = 0; y < size.y; y++) {
    const unsigned char *in =
        (const unsigned char *)&pixel[(size.y - 1 - y) * size.x];
    for (int x = 0; x < size.x; x++) {
      out[3 * x + 0] = in[4 * x + 0];
      out[3 * x + 1] = in[4 * x + 1];
      out[3 * x + 2] = in[4 * x + 2];
    }
    fwrite(out, 3 * size.x, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}

int main(int argc, char **argv)
{
  int mpiThreadCapability = 0;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpiThreadCapability);
  if (mpiThreadCapability != MPI_THREAD_MULTIPLE
      && mpiThreadCapability != MPI_THREAD_SERIALIZED) {
    fprintf(stderr,
        "OSPRay requires the MPI runtime to support thread "
        "multiple or thread serialized.\n");
    return 1;
  }

  int mpiRank = 0;
  int mpiWorldSize = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiWorldSize);

  // image size
  vec2i imgSize;
  imgSize.x = 1024; // width
  imgSize.y = 768; // height

  // camera
  vec3f cam_pos{(mpiWorldSize + 1.f) / 2.f, 0.5f, -mpiWorldSize * 0.5f};
  vec3f cam_up{0.f, 1.f, 0.f};
  vec3f cam_view{0.f, 0.f, 1.f};

  // all ranks specify the same rendering parameters, with the exception of
  // the data to be rendered, which is distributed among the ranks
  // triangle mesh data
  float vertex[] = {mpiRank,
      0.0f,
      3.5f,
      mpiRank,
      1.0f,
      3.0f,
      1.0f * (mpiRank + 1.f),
      0.0f,
      3.0f,
      1.0f * (mpiRank + 1.f),
      1.0f,
      2.5f};
  float color[] = {0.0f,
      0.0f,
      (mpiRank + 1.f) / mpiWorldSize,
      1.0f,
      0.0f,
      0.0f,
      (mpiRank + 1.f) / mpiWorldSize,
      1.0f,
      0.0f,
      0.0f,
      (mpiRank + 1.f) / mpiWorldSize,
      1.0f,
      0.0f,
      0.0f,
      (mpiRank + 1.f) / mpiWorldSize,
      1.0f};
  int32_t index[] = {0, 1, 2, 1, 2, 3};

  // load the MPI module, and select the MPI distributed device. Here we
  // do not call ospInit, as we want to explicitly pick the distributed
  // device. This can also be done by passing --osp:mpi-distributed when
  // using ospInit, however if the user doesn't pass this argument your
  // application will likely not behave as expected
  ospLoadModule("mpi");

  OSPDevice mpiDevice = ospNewDevice("mpi_distributed");
  ospDeviceCommit(mpiDevice);
  ospSetCurrentDevice(mpiDevice);

  // use scoped lifetimes of wrappers to release everything before ospShutdown()
  {
    // create and setup camera
    ospray::cpp::Camera camera("perspective");
    camera.setParam("aspect", imgSize.x / (float)imgSize.y);
    camera.setParam("position", cam_pos);
    camera.setParam("direction", cam_view);
    camera.setParam("up", cam_up);
    camera.commit(); // commit each object to indicate modifications are done

    // create and setup model and mesh
    ospray::cpp::Geometry mesh("triangles");
    ospray::cpp::Data data(4,
        OSP_VEC3F,
        vertex); // OSP_FLOAT3 format is also supported for vertex positions
    data.commit();
    mesh.setParam("vertex.position", data);

    data = ospray::cpp::Data(4, OSP_VEC4F, color);
    data.commit();
    mesh.setParam("vertex.color", data);

    data = ospray::cpp::Data(2, OSP_VEC3UI, index);
    data.commit();
    mesh.setParam("index", data);

    mesh.commit();

    // put the mesh into a model
    ospray::cpp::GeometricModel model(mesh);
    model.commit();

    // put the model into a group (collection of models)
    ospray::cpp::Group group;
    auto modelHandle = model.handle();
    data = ospray::cpp::Data(1, OSP_GEOMETRIC_MODEL, &modelHandle);
    group.setParam("geometry", data);
    group.commit();

    // put the group into an instance (give the group a world transform)
    ospray::cpp::Instance instance(group);
    instance.commit();

    ospray::cpp::World world;
    auto instanceHandle = instance.handle();
    data = ospray::cpp::Data(1, OSP_INSTANCE, &instanceHandle);
    world.setParam("instance", data);

    // Specify the region of the world this rank owns
    float regionBounds[] = {mpiRank, 0.f, 2.5f, 1.f * (mpiRank + 1.f), 1.f, 3.5f};
    data = ospray::cpp::Data(1, OSP_BOX3F, regionBounds);
    data.commit();
    world.setParam("regions", data);

    world.commit();

    // create the mpi_raycast renderer (requred for distributed rendering)
    ospray::cpp::Renderer renderer("mpi_raycast");

    // create and setup light for Ambient Occlusion
    // TODO: Who gets the lights now?
    ospray::cpp::Light light("ambient");
    light.commit();
    auto lightHandle = light.handle();
    ospray::cpp::Data lights(1, OSP_LIGHT, &lightHandle);
    lights.commit();

    // complete setup of renderer
    renderer.setParam("bgColor", 1.0f); // white, transparent
    renderer.setParam("light", lights);
    renderer.commit();

    // create and setup framebuffer
    ospray::cpp::FrameBuffer framebuffer(
        imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);
    framebuffer.clear();

    // render one frame
    framebuffer.renderFrame(renderer, camera, world);

    // on rank 0, access framebuffer and write its content as PPM file
    if (mpiRank == 0) {
      uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
      writePPM("firstFrameCpp.ppm", imgSize, fb);
      framebuffer.unmap(fb);
    }

    // render 10 more frames, which are accumulated to result in a better
    // converged image
    for (int frames = 0; frames < 10; frames++)
      framebuffer.renderFrame(renderer, camera, world);

    if (mpiRank == 0) {
      uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
      writePPM("accumulatedFrameCpp.ppm", imgSize, fb);
      framebuffer.unmap(fb);
    }
  }

  ospShutdown();

  MPI_Finalize();

  return 0;
}
