// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
 * Build it in the build_directory with
 *   g++ ../apps/ospTutorial.cpp -I ../ospray/include -I .. -I ../ospray/embree/common  ./libospray.so -o ospTutorial
 */

#include "ospray/ospray.h"

// helper function to write the rendered image as PPM file
void writePPM(const char *fileName,
              const int sizeX, const int sizeY,
              const uint32 *pixel)
{
  FILE *file = fopen(fileName, "wb");
  fprintf(file, "P6\n%i %i\n255\n", sizeX, sizeY);
  unsigned char out[3*sizeX];
  for (int y = 0; y < sizeY; y++) {
    const unsigned char *in = (const unsigned char *)&pixel[(sizeY-1-y)*sizeX];
    for (int x = 0; x < sizeX; x++) {
      out[3*x + 0] = in[4*x + 0];
      out[3*x + 1] = in[4*x + 1];
      out[3*x + 2] = in[4*x +2 ];
    }
    fwrite(&out, 3*sizeX, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}


int main(int ac, const char **av) {
  // image size
  int width = 1024;
  int height = 768;

  // camera
  osp::vec3f cam_pos(0.f);
  osp::vec3f cam_up(0.f, 1.f, 0.f);
  osp::vec3f cam_view(0.1f, 0.f, 1.f);

  // triangle mesh data
  float vertex[] = { -1.0f, -1.0f, 3.0f, 0.f,
                     -1.0f,  1.0f, 3.0f, 0.f,
                      1.0f, -1.0f, 3.0f, 0.f,
                      0.1f,  0.1f, 0.3f, 0.f };
  float color[] =  { 0.9f, 0.5f, 0.5f, 1.0f,
                     0.8f, 0.8f, 0.8f, 1.0f,
                     0.8f, 0.8f, 0.8f, 1.0f,
                     0.5f, 0.9f, 0.5f, 1.0f };
  int32 index[] = { 0, 1, 2,
                    1, 2, 3 };


  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters, e.g. "--osp:debug"
  ospInit(&ac, av);

  // create and setup camera
  OSPCamera camera = ospNewCamera("perspective");
  ospSetf(camera, "aspect", width/(float)height);
  ospSetVec3f(camera, "pos", cam_pos);
  ospSetVec3f(camera, "dir", cam_view);
  ospSetVec3f(camera, "up",  cam_up);
  ospCommit(camera); // commit each object to indicate modifications are done


  // create and setup model and mesh
  OSPGeometry mesh = ospNewTriangleMesh();
  OSPData data = ospNewData(4, OSP_FLOAT3A, vertex); // OSP_FLOAT3 format is also supported for vertex positions (currently not on MIC)
  ospCommit(data);
  ospSetData(mesh, "vertex", data);

  data = ospNewData(4, OSP_FLOAT4, color);
  ospCommit(data);
  ospSetData(mesh, "vertex.color", data);

  data = ospNewData(2, OSP_INT3, index); // OSP_INT4 format is also supported for triangle indices
  ospCommit(data);
  ospSetData(mesh, "index", data);

  ospCommit(mesh);


  OSPModel world = ospNewModel();
  ospAddGeometry(world, mesh);
  ospCommit(world);


  // create and setup renderer
  OSPRenderer renderer = ospNewRenderer("ao4"); // choose Ambient Occlusion renderer
  ospSetObject(renderer, "model",  world);
  ospSetObject(renderer, "camera", camera);
  ospCommit(renderer);


  // create and setup framebuffer
  OSPFrameBuffer framebuffer = ospNewFrameBuffer(osp::vec2i(width, height), OSP_RGBA_I8, OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);
  ospFrameBufferClear(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

  // render one frame
  ospRenderFrame(framebuffer, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);

  // access framebuffer and write its content as PPM file
  const uint32 * fb = (uint32*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
  writePPM("firstFrame.ppm", width, height, fb);
  ospUnmapFrameBuffer(fb, framebuffer);


  // render 10 more frames, which are accumulated to result in a better converged image
  for (int frames = 0; frames < 10; frames++)
    ospRenderFrame(framebuffer, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);

  fb = (uint32*)ospMapFrameBuffer(framebuffer, OSP_FB_COLOR);
  writePPM("accumulatedFrame.ppm", width, height, fb);
  ospUnmapFrameBuffer(fb, framebuffer);

  return 0;
}
