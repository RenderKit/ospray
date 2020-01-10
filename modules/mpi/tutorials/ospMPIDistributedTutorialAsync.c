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
 *   mpicc -std=c99 ../modules/mpi/tutorials/ospMPIDistributedTutorialAsync.c \
 *       -I ../ospray/include \
 *       -L . -lospray -Wl,-rpath,. \
 *       -o ospMPIDistributedTutorialAsync
 *
 * Then run it with MPI on some number of processes
 *   mpirun -n <N> ./ospMPIDistributedTutorialAsync
 *
 * The output image should show a sequence of quads, from dark to light blue
 */

#include <errno.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include <ospray/ospray.h>
#include <ospray/ospray_util.h>

// helper function to write the rendered image as PPM file
void writePPM(
    const char *fileName, int size_x, int size_y, const uint32_t *pixel)
{
  FILE *file = fopen(fileName, "wb");
  if (!file) {
    fprintf(stderr, "fopen('%s', 'wb') failed: %d", fileName, errno);
    return;
  }
  fprintf(file, "P6\n%i %i\n255\n", size_x, size_y);
  unsigned char *out = (unsigned char *)alloca(3 * size_x);
  for (int y = 0; y < size_y; y++) {
    const unsigned char *in =
        (const unsigned char *)&pixel[(size_y - 1 - y) * size_x];
    for (int x = 0; x < size_x; x++) {
      out[3 * x + 0] = in[4 * x + 0];
      out[3 * x + 1] = in[4 * x + 1];
      out[3 * x + 2] = in[4 * x + 2];
    }
    fwrite(out, 3 * size_x, sizeof(char), file);
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
  int imgSizeX = 1024; // width
  int imgSizeY = 768; // height

  // camera
  float cam_pos[] = {(mpiWorldSize + 1.f) / 2.f, 0.5f, -mpiWorldSize * 0.5f};
  float cam_up[] = {0.f, 1.f, 0.f};
  float cam_view[] = {0.0f, 0.f, 1.f};

  // all ranks specify the same rendering parameters, with the exception of
  // the data to be rendered, which is distributed among the ranks
  // triangle mesh data
  float vertex[] = {
      mpiRank,
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
      2.5f,
  };
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

  OSPDevice mpiDevice = ospNewDevice("mpiDistributed");
  ospDeviceCommit(mpiDevice);
  ospSetCurrentDevice(mpiDevice);

  // create and setup camera
  OSPCamera camera = ospNewCamera("perspective");
  ospSetFloat(camera, "aspect", imgSizeX / (float)imgSizeY);
  ospSetParam(camera, "position", OSP_VEC3F, cam_pos);
  ospSetParam(camera, "direction", OSP_VEC3F, cam_view);
  ospSetParam(camera, "up", OSP_VEC3F, cam_up);
  ospCommit(camera); // commit each object to indicate modifications are done

  // create and setup model and mesh
  OSPGeometry mesh = ospNewGeometry("triangles");
  OSPData data = ospNewSharedData1D(vertex, OSP_VEC3F, 4);
  ospCommit(data);
  ospSetObject(mesh, "vertex.position", data);
  ospRelease(data); // we are done using this handle

  data = ospNewSharedData1D(color, OSP_VEC4F, 4);
  ospCommit(data);
  ospSetObject(mesh, "vertex.color", data);
  ospRelease(data); // we are done using this handle

  data = ospNewSharedData1D(index, OSP_VEC3UI, 2);
  ospCommit(data);
  ospSetObject(mesh, "index", data);
  ospRelease(data); // we are done using this handle

  ospCommit(mesh);

  // put the mesh into a model
  OSPGeometricModel model = ospNewGeometricModel(mesh);
  ospCommit(model);
  ospRelease(mesh);

  // put the model into a group (collection of models)
  OSPGroup group = ospNewGroup();
  OSPData geometricModels = ospNewSharedData1D(&model, OSP_GEOMETRIC_MODEL, 1);
  ospSetObject(group, "geometry", geometricModels);
  ospCommit(group);
  ospRelease(model);
  ospRelease(geometricModels);

  // put the group into an instance (give the group a world transform)
  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);
  ospRelease(group);

  // put the instance in the world
  OSPWorld world = ospNewWorld();
  OSPData instances = ospNewSharedData1D(&instance, OSP_INSTANCE, 1);
  ospSetObject(world, "instance", instances);
  ospRelease(instance);
  ospRelease(instances);

  // In the distributed device we set a clipping region to clip to the data
  // owned uniquely by this rank which it should be rendering
  float regionBounds[] = {mpiRank, 0.f, 2.5f, 1.f * (mpiRank + 1.f), 1.f, 3.5f};
  data = ospNewSharedData1D(regionBounds, OSP_BOX3F, 1);
  ospCommit(data);
  ospSetObject(world, "regions", data);
  ospRelease(data);

  ospCommit(world);

  // create the mpi_raycast renderer (requred for distributed rendering)
  OSPRenderer renderer = ospNewRenderer("mpiRaycast");

  // create and setup light for Ambient Occlusion
  // TODO: Who gets the lights now?
  OSPLight light = ospNewLight("ambient");
  ospCommit(light);
  OSPData lights = ospNewSharedData1D(&light, OSP_LIGHT, 1);
  ospCommit(lights);

  // complete setup of renderer
  ospSetFloat(renderer, "backgroundColor", 1.0f); // white, transparent
  ospSetObject(renderer, "light", lights);
  ospCommit(renderer);

  // create and setup framebuffer
  OSPFrameBuffer framebuffer = ospNewFrameBuffer(imgSizeX,
      imgSizeY,
      OSP_FB_SRGBA,
      OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);
  ospResetAccumulation(framebuffer);

  // render one frame
  OSPFuture result = ospRenderFrame(framebuffer, renderer, camera, world);
  int isFinished = ospIsReady(result, OSP_TASK_FINISHED);
  printf("status of 'result' is %i\n", isFinished);
  printf("waiting on frame to finish...\n");
  ospWait(result, OSP_FRAME_FINISHED);
  printf("...done!\n");
  printf("variance was %f\n", ospGetVariance(framebuffer));
  ospRelease(result);

  // on rank 0, access framebuffer and write its content as PPM file
  if (mpiRank == 0) {
    const uint32_t *fb =
        (uint32_t *)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
    writePPM("firstFrame.ppm", imgSizeX, imgSizeY, fb);
    ospUnmapFrameBuffer(fb, framebuffer);
  }

  // render 10 more frames, which are accumulated to result in a better
  // converged image
  for (int frames = 0; frames < 10; frames++)
    ospRenderFrameBlocking(framebuffer, renderer, camera, world);

  if (mpiRank == 0) {
    const uint32_t *fb =
        (uint32_t *)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
    writePPM("accumulatedFrame.ppm", imgSizeX, imgSizeY, fb);
    ospUnmapFrameBuffer(fb, framebuffer);
  }

  // final cleanups
  ospRelease(renderer);
  ospRelease(camera);
  ospRelease(lights);
  ospRelease(light);
  ospRelease(framebuffer);
  ospRelease(world);

  ospShutdown();

  MPI_Finalize();

  return 0;
}

