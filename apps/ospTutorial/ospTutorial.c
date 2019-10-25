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


/* This is a small example tutorial how to use OSPRay in an application.
 *
 * On Linux build it in the build_directory with
 *   gcc -std=c99 ../apps/tutorials/ospTutorial.c \
 *       -I ../ospray/include -L . -lospray -Wl,-rpath,. -o ospTutorial
 * On Windows build it in the build_directory\$Configuration with
 *   cl ..\..\apps\tutorials\ospTutorial.c -I ..\..\ospray\include -I ..\.. ospray.lib
 */

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#ifdef _WIN32
#  include <malloc.h>
#else
#  include <alloca.h>
#endif
#include "ospray/ospray_util.h"

// helper function to write the rendered image as PPM file
void writePPM(const char *fileName,
              int size_x,
              int size_y,
              const uint32_t *pixel)
{
  FILE *file = fopen(fileName, "wb");
  if (!file) {
    fprintf(stderr, "fopen('%s', 'wb') failed: %d", fileName, errno);
    return;
  }
  fprintf(file, "P6\n%i %i\n255\n", size_x, size_y);
  unsigned char *out = (unsigned char *)alloca(3*size_x);
  for (int y = 0; y < size_y; y++) {
    const unsigned char *in = (const unsigned char *)&pixel[(size_y-1-y)*size_x];
    for (int x = 0; x < size_x; x++) {
      out[3*x + 0] = in[4*x + 0];
      out[3*x + 1] = in[4*x + 1];
      out[3*x + 2] = in[4*x + 2];
    }
    fwrite(out, 3*size_x, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}


int main(int argc, const char **argv) {
  // image size
  int imgSize_x = 1024; // width
  int imgSize_y = 768; // height

  // camera
  float cam_pos[] = {0.f, 0.f, 0.f};
  float cam_up [] = {0.f, 1.f, 0.f};
  float cam_view [] = {0.1f, 0.f, 1.f};

  // triangle mesh data
  float vertex[] = { -1.0f, -1.0f, 3.0f,
                     -1.0f,  1.0f, 3.0f,
                      1.0f, -1.0f, 3.0f,
                      0.1f,  0.1f, 0.3f };
  float color[] =  { 0.9f, 0.5f, 0.5f, 1.0f,
                     0.8f, 0.8f, 0.8f, 1.0f,
                     0.8f, 0.8f, 0.8f, 1.0f,
                     0.5f, 0.9f, 0.5f, 1.0f };
  int32_t index[] = { 0, 1, 2,
                      1, 2, 3 };

  printf("initialize OSPRay...");

  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters, e.g. "--osp:debug"
  OSPError init_error = ospInit(&argc, argv);
  if (init_error != OSP_NO_ERROR)
    return init_error;

  printf("done!\n");
  printf("setting up camera...");

  // create and setup camera
  OSPCamera camera = ospNewCamera("perspective");
  ospSetFloat(camera, "aspect", imgSize_x/(float)imgSize_y);
  ospSetParam(camera, "position", OSP_VEC3F, cam_pos);
  ospSetParam(camera, "direction", OSP_VEC3F, cam_view);
  ospSetParam(camera, "up", OSP_VEC3F, cam_up);
  ospCommit(camera); // commit each object to indicate modifications are done

  printf("done!\n");
  printf("setting up scene...");

  // create and setup model and mesh
  OSPGeometry mesh = ospNewGeometry("triangles");

  OSPData data = ospNewSharedData1D(vertex, OSP_VEC3F, 4);
  // alternatively with an OSPRay managed OSPData
  // OSPData managed = ospNewData1D(OSP_VEC3F, 4);
  // ospCopyData1D(data, managed, 0);

  ospCommit(data);
  ospSetObject(mesh, "vertex.position", data);
  ospRelease(data); // we are done using this handle

  data = ospNewSharedData1D(color, OSP_VEC4F, 4);
  ospCommit(data);
  ospSetObject(mesh, "vertex.color", data);
  ospRelease(data);

  data = ospNewSharedData1D(index, OSP_VEC3UI, 2);
  ospCommit(data);
  ospSetObject(mesh, "index", data);
  ospRelease(data);

  ospCommit(mesh);

  OSPMaterial mat = ospNewMaterial("pathtracer", "OBJMaterial");
  ospCommit(mat);

  // put the mesh into a model
  OSPGeometricModel model = ospNewGeometricModel(mesh);
  ospSetObjectAsData(model, "material", OSP_MATERIAL, mat);
  ospCommit(model);
  ospRelease(mesh);
  ospRelease(mat);

  // put the model into a group (collection of models)
  OSPGroup group = ospNewGroup();
  ospSetObjectAsData(group, "geometry", OSP_GEOMETRIC_MODEL, model);
  ospCommit(group);
  ospRelease(model);

  // put the group into an instance (give the group a world transform)
  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);
  ospRelease(group);

  // put the instance in the world
  OSPWorld world = ospNewWorld();
  ospSetObjectAsData(world, "instance", OSP_INSTANCE, instance);
  ospRelease(instance);

  // create and setup light for Ambient Occlusion
  OSPLight light = ospNewLight("ambient");
  ospCommit(light);
  ospSetObjectAsData(world, "light", OSP_LIGHT, light);
  ospRelease(light);

  ospCommit(world);

  printf("done!\n");

  // print out world bounds
  OSPBounds worldBounds = ospGetBounds(world);
  printf("\nworld bounds: ({%f, %f, %f}, {%f, %f, %f}\n\n",
         worldBounds.lower[0], worldBounds.lower[1], worldBounds.lower[2],
         worldBounds.upper[0], worldBounds.upper[1], worldBounds.upper[2]);

  printf("setting up renderer...");

  // create renderer
  OSPRenderer renderer =
      ospNewRenderer("pathtracer"); // choose path tracing renderer

  // complete setup of renderer
  ospSetInt(renderer, "aoSamples", 1);
  ospSetFloat(renderer, "bgColor", 1.0f); // white, transparent
  ospCommit(renderer);

  // create and setup framebuffer
  OSPFrameBuffer framebuffer = ospNewFrameBuffer(imgSize_x, imgSize_y, OSP_FB_SRGBA, OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);
  ospResetAccumulation(framebuffer);

  printf("rendering initial frame to firstFrame.ppm...");

  // render one frame
  ospRenderFrameBlocking(framebuffer, renderer, camera, world);

  // access framebuffer and write its content as PPM file
  const uint32_t * fb = (uint32_t*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
  writePPM("firstFrame.ppm", imgSize_x, imgSize_y, fb);
  ospUnmapFrameBuffer(fb, framebuffer);

  printf("done!\n");
  printf("rendering 10 accumulated frames to accumulatedFrame.ppm...");

  // render 10 more frames, which are accumulated to result in a better converged image
  for (int frames = 0; frames < 10; frames++)
    ospRenderFrameBlocking(framebuffer, renderer, camera, world);

  fb = (uint32_t*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
  writePPM("accumulatedFrame.ppm", imgSize_x, imgSize_y, fb);
  ospUnmapFrameBuffer(fb, framebuffer);

  printf("done!\n");

  OSPPickResult p;
  ospPick(&p, framebuffer, renderer, camera, world, 0.5f, 0.5f);

  printf("\nospPick() center of screen --> [inst: %p, model: %p, prim: %u]\n",
         p.instance,
         p.model,
         p.primID);

  printf("\ncleaning up objects...");

  // cleanup pick handles (because p.hasHit was 'true')
  ospRelease(p.instance);
  ospRelease(p.model);

  // final cleanups
  ospRelease(renderer);
  ospRelease(camera);
  ospRelease(framebuffer);
  ospRelease(world);

  printf("done!\n");

  ospShutdown();

  return 0;
}
