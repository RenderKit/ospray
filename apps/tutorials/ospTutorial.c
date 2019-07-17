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
#include "ospray/ospray.h"

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
  float vertex[] = { -1.0f, -1.0f, 3.0f, 0.f,
                     -1.0f,  1.0f, 3.0f, 0.f,
                      1.0f, -1.0f, 3.0f, 0.f,
                      0.1f,  0.1f, 0.3f, 0.f };
  float color[] =  { 0.9f, 0.5f, 0.5f, 1.0f,
                     0.8f, 0.8f, 0.8f, 1.0f,
                     0.8f, 0.8f, 0.8f, 1.0f,
                     0.5f, 0.9f, 0.5f, 1.0f };
  int32_t index[] = { 0, 1, 2,
                      1, 2, 3 };


  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters, e.g. "--osp:debug"
  OSPError init_error = ospInit(&argc, argv);
  if (init_error != OSP_NO_ERROR)
    return init_error;

  // create and setup camera
  OSPCamera camera = ospNewCamera("perspective");
  ospSetFloat(camera, "aspect", imgSize_x/(float)imgSize_y);
  ospSetVec3fv(camera, "position", cam_pos);
  ospSetVec3fv(camera, "direction", cam_view);
  ospSetVec3fv(camera, "up",  cam_up);
  ospCommit(camera); // commit each object to indicate modifications are done


  // create and setup model and mesh
  OSPGeometry mesh = ospNewGeometry("triangles");
  OSPData data = ospNewData(4, OSP_VEC3FA, vertex, 0); // OSP_VEC3F format is also supported for vertex positions
  ospCommit(data);
  ospSetData(mesh, "vertex.position", data);
  ospRelease(data); // we are done using this handle

  data = ospNewData(4, OSP_VEC4F, color, 0);
  ospCommit(data);
  ospSetData(mesh, "vertex.color", data);
  ospRelease(data); // we are done using this handle

  data = ospNewData(2, OSP_VEC3I, index, 0); // OSP_VEC4I format is also supported for triangle indices
  ospCommit(data);
  ospSetData(mesh, "index", data);
  ospRelease(data); // we are done using this handle

  ospCommit(mesh);

  // put the mesh into a model
  OSPGeometricModel model = ospNewGeometricModel(mesh);
  ospCommit(model);
  ospRelease(mesh); // we are done using this handle

  // put the model into a group (collection of models)
  OSPGroup group = ospNewGroup();
  OSPData geometricModels = ospNewData(1, OSP_OBJECT, &model, 0);
  ospSetData(group, "geometry", geometricModels);
  ospCommit(group);
  ospRelease(model);
  ospRelease(geometricModels);

  // put the group into an instance (give the group a world transform)
  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);
  ospRelease(group);

  // put the instance in the world
  OSPWorld world = ospNewWorld();
  OSPData instances = ospNewData(1, OSP_OBJECT, &instance, 0);
  ospSetData(world, "instance", instances);
  ospCommit(world);
  ospRelease(instance);
  ospRelease(instances);

  // create renderer
  OSPRenderer renderer = ospNewRenderer("scivis"); // choose Scientific Visualization renderer

  // create and setup light for Ambient Occlusion
  OSPLight light = ospNewLight("ambient");
  ospCommit(light);
  OSPData lights = ospNewData(1, OSP_LIGHT, &light, 0);
  ospCommit(lights);

  // complete setup of renderer
  ospSetInt(renderer, "aoSamples", 1);
  ospSetFloat(renderer, "bgColor", 1.0f); // white, transparent
  ospSetObject(renderer, "light", lights);
  ospCommit(renderer);

  // create and setup framebuffer
  OSPFrameBuffer framebuffer = ospNewFrameBuffer(imgSize_x, imgSize_y, OSP_FB_SRGBA, OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);
  ospResetAccumulation(framebuffer);

  // render one frame
  ospRenderFrame(framebuffer, renderer, camera, world);

  // access framebuffer and write its content as PPM file
  const uint32_t * fb = (uint32_t*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
  writePPM("firstFrame.ppm", imgSize_x, imgSize_y, fb);
  ospUnmapFrameBuffer(fb, framebuffer);

  // render 10 more frames, which are accumulated to result in a better converged image
  for (int frames = 0; frames < 10; frames++)
    ospRenderFrame(framebuffer, renderer, camera, world);

  fb = (uint32_t*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
  writePPM("accumulatedFrame.ppm", imgSize_x, imgSize_y, fb);
  ospUnmapFrameBuffer(fb, framebuffer);

  // final cleanups
  ospRelease(renderer);
  ospRelease(camera);
  ospRelease(lights);
  ospRelease(light);
  ospRelease(framebuffer);
  ospRelease(world);

  ospShutdown();

  return 0;
}
