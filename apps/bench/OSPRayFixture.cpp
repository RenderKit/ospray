// ======================================================================== //
// Copyright 2016 Intel Corporation                                         //
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

#include "OSPRayFixture.h"

using std::string;

using namespace ospcommon;

std::unique_ptr<ospray::cpp::Renderer>    OSPRayFixture::renderer;
std::unique_ptr<ospray::cpp::Camera>      OSPRayFixture::camera;
std::unique_ptr<ospray::cpp::Model>       OSPRayFixture::model;
std::unique_ptr<ospray::cpp::FrameBuffer> OSPRayFixture::fb;

string OSPRayFixture::imageOutputFile;

std::vector<string> OSPRayFixture::benchmarkModelFiles;

int OSPRayFixture::width  = 1024;
int OSPRayFixture::height = 1024;

int OSPRayFixture::numWarmupFrames = 10;

vec3f OSPRayFixture::bg_color = {1.f, 1.f, 1.f};

// helper function to write the rendered image as PPM file
static void writePPM(const string &fileName, const int sizeX, const int sizeY,
                     const uint32_t *pixel)
{
  FILE *file = fopen(fileName.c_str(), "wb");
  fprintf(file, "P6\n%i %i\n255\n", sizeX, sizeY);
  unsigned char *out = (unsigned char *)alloca(3*sizeX);
  for (int y = 0; y < sizeY; y++) {
    const unsigned char *in = (const unsigned char *)&pixel[(sizeY-1-y)*sizeX];
    for (int x = 0; x < sizeX; x++) {
      out[3*x + 0] = in[4*x + 0];
      out[3*x + 1] = in[4*x + 1];
      out[3*x + 2] = in[4*x + 2];
    }
    fwrite(out, 3*sizeX, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}

static void createFramebuffer(OSPRayFixture *f)
{
  *f->fb = ospray::cpp::FrameBuffer(osp::vec2i{f->width, f->height},
                                    OSP_FB_SRGBA,
                                    OSP_FB_COLOR|OSP_FB_ACCUM);
  f->fb->clear(OSP_FB_ACCUM | OSP_FB_COLOR);
}

void OSPRayFixture::SetUp()
{
  createFramebuffer(this);

  renderer->set("world",  *model);
  renderer->set("model",  *model);
  renderer->set("camera", *camera);
  renderer->set("spp", 1);

  renderer->commit();

  for (int i = 0; i < numWarmupFrames; ++i) {
    renderer->renderFrame(*fb, OSP_FB_COLOR | OSP_FB_ACCUM);
  }
}

void OSPRayFixture::TearDown()
{
  if (!imageOutputFile.empty()) {
    auto *lfb = (uint32_t*)fb->map(OSP_FB_COLOR);
    writePPM(imageOutputFile + ".ppm", width, height, lfb);
    fb->unmap(lfb);
  }
}
