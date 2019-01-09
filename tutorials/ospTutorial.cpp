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
 *   g++ ../apps/ospTutorial.cpp -I ../ospray/include -I .. ./libospray.so -Wl,-rpath,. -o ospTutorial
 * On Windows build it in the build_directory\$Configuration with
 *   cl ..\..\apps\ospTutorial.cpp /EHsc -I ..\..\ospray\include -I ..\.. -I ..\..\components ospray.lib
 */

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#ifdef _WIN32
#  define NOMINMAX
#  include <malloc.h>
#else
#  include <alloca.h>
#endif

#include "ospray/ospray_cpp.h"

// helper function to write the rendered image as PPM file
void writePPM(const char *fileName,
              const ospcommon::vec2i &size,
              const uint32_t *pixel)
{
  FILE *file = fopen(fileName, "wb");
  if(file == nullptr) {
    fprintf(stderr, "fopen('%s', 'wb') failed: %d", fileName, errno);
    return;
  }
  fprintf(file, "P6\n%i %i\n255\n", size.x, size.y);
  unsigned char *out = (unsigned char *)alloca(3*size.x);
  for (int y = 0; y < size.y; y++) {
    const unsigned char *in = (const unsigned char *)&pixel[(size.y-1-y)*size.x];
    for (int x = 0; x < size.x; x++) {
      out[3*x + 0] = in[4*x + 0];
      out[3*x + 1] = in[4*x + 1];
      out[3*x + 2] = in[4*x + 2];
    }
    fwrite(out, 3*size.x, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}


int main(int argc, const char **argv) {
  // image size
  ospcommon::vec2i imgSize;
  imgSize.x = 1024; // width
  imgSize.y = 768; // height

  // camera
  ospcommon::vec3f cam_pos{0.f, 0.f, 0.f};
  ospcommon::vec3f cam_up{0.f, 1.f, 0.f};
  ospcommon::vec3f cam_view{0.1f, 0.f, 1.f};

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
  ospray::cpp::Camera camera("perspective");
  camera.set("aspect", imgSize.x/(float)imgSize.y);
  camera.set("pos", cam_pos);
  camera.set("dir", cam_view);
  camera.set("up",  cam_up);
  camera.commit(); // commit each object to indicate modifications are done

  // create and setup model and mesh
  ospray::cpp::Geometry mesh("triangles");
  ospray::cpp::Data data(4, OSP_FLOAT3A, vertex); // OSP_FLOAT3 format is also supported for vertex positions
  data.commit();
  mesh.set("vertex", data);
  data.release(); // we are done using this handle

  data = ospray::cpp::Data(4, OSP_FLOAT4, color);
  data.commit();
  mesh.set("vertex.color", data);
  data.release(); // we are done using this handle

  data = ospray::cpp::Data(2, OSP_INT3, index); // OSP_INT4 format is also supported for triangle indices
  data.commit();
  mesh.set("index", data);
  data.release(); // we are done using this handle

  mesh.commit();

  ospray::cpp::Model world;
  world.addGeometry(mesh);
  mesh.release(); // we are done using this handle
  world.commit();

  // create renderer
  ospray::cpp::Renderer renderer("scivis"); // choose Scientific Visualization renderer

  // create and setup light for Ambient Occlusion
  ospray::cpp::Light light("ambient");
  light.commit();
  auto lightHandle = light.handle();
  ospray::cpp::Data lights(1, OSP_LIGHT, &lightHandle);
  lights.commit();

  // complete setup of renderer
  renderer.set("aoSamples", 1);
  renderer.set("bgColor", 1.0f); // white, transparent
  renderer.set("model",  world);
  renderer.set("camera", camera);
  renderer.set("lights", lights);
  renderer.commit();


  // create and setup framebuffer
  ospray::cpp::FrameBuffer framebuffer(imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);
  framebuffer.clear(OSP_FB_COLOR | OSP_FB_ACCUM);

  // render one frame
  renderer.renderFrame(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

  // access framebuffer and write its content as PPM file
  uint32_t* fb = (uint32_t*)framebuffer.map(OSP_FB_COLOR);
  writePPM("firstFrameCpp.ppm", imgSize, fb);
  framebuffer.unmap(fb);


  // render 10 more frames, which are accumulated to result in a better converged image
  for (int frames = 0; frames < 10; frames++)
    renderer.renderFrame(framebuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

  fb = (uint32_t*)framebuffer.map(OSP_FB_COLOR);
  writePPM("accumulatedFrameCpp.ppm", imgSize, fb);
  framebuffer.unmap(fb);

  // final cleanups
  renderer.release();
  camera.release();
  lights.release();
  light.release();
  framebuffer.release();
  world.release();

  ospShutdown();

  return 0;
}
