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

using namespace ospcommon;
using namespace ospray;

const static int DEFAULT_WIDTH = 1024;
const static int DEFAULT_HEIGHT = 1024;
const static size_t DEFAULT_BENCH_FRAMES = 100;
const static size_t DEFAULT_WARMUP_FRAMES = 10;

const static vec3f DEFAULT_BG_COLOR = {1.f, 1.f, 1.f};

namespace bench {
// helper function to write the rendered image as PPM file
void writePPM(const std::string &fileName, const int sizeX, const int sizeY,
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
}

OSPRayFixture::OSPRayFixture(cpp::Renderer r, cpp::Camera c, cpp::Model m)
  : renderer(r), camera(c), model(m), width(DEFAULT_WIDTH), height(DEFAULT_HEIGHT),
  defaultBenchFrames(DEFAULT_BENCH_FRAMES), defaultWarmupFrames(DEFAULT_WARMUP_FRAMES)
{
  setFrameBuffer(width, height);
  renderer.set("world", model);
  renderer.set("model", model);
  renderer.set("camera", camera);
  renderer.commit();
}
pico_bench::Statistics<OSPRayFixture::Millis>
OSPRayFixture::benchmark(const size_t warmUpFrames, const size_t benchFrames) {
  const size_t warmup = warmUpFrames == 0 ? defaultWarmupFrames : warmUpFrames;
  const size_t bench = benchFrames == 0 ? defaultBenchFrames : benchFrames;
  for (size_t i = 0; i < warmup; ++i) {
    renderer.renderFrame(fb, framebufferFlags);
  }
  fb.clear(framebufferFlags);

  auto benchmarker = pico_bench::Benchmarker<Millis>(bench);
  auto stats = benchmarker([&]() {
      renderer.renderFrame(fb, framebufferFlags);
  });
  stats.time_suffix = " ms";
  return stats;
}
void OSPRayFixture::saveImage(const std::string &fname) {
  auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
  bench::writePPM(fname + ".ppm", width, height, lfb);
  fb.unmap(lfb);
}
void OSPRayFixture::setFrameBuffer(const int w, const int h, const int fbFlags) {
  width = w > 0 ? w : width;
  height = h > 0 ? h : height;

  fb = cpp::FrameBuffer(osp::vec2i{width, height}, OSP_FB_SRGBA, fbFlags);
  fb.clear(fbFlags);
  framebufferFlags = fbFlags;

  camera.set("aspect", static_cast<float>(width) / height);
  camera.commit();
}

