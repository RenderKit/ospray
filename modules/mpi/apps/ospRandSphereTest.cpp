// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

// ospcommon
#include "ospcommon/utility/SaveImage.h"
// mpiCommon
#include "mpiCommon/MPICommon.h"
// public-ospray
#include "ospray/ospray_cpp/Camera.h"
#include "ospray/ospray_cpp/Data.h"
#include "ospray/ospray_cpp/Device.h"
#include "ospray/ospray_cpp/Model.h"
#include "ospray/ospray_cpp/FrameBuffer.h"
#include "ospray/ospray_cpp/Renderer.h"
#include "ospray/ospray_cpp/TransferFunction.h"
// pico_bench
#include "apps/bench/pico_bench/pico_bench.h"
// stl
#include <random>

namespace ospRandSphereTest {

  using namespace ospcommon;

  static int   numSpheresPerNode = 100;
  static float sphereRadius      = 0.01f;
  static float sceneLowerBound   = 0.f;
  static float sceneUpperBound   = 1.f;
  static vec2i fbSize            = vec2i(1024, 768);
  static int   numFrames         = 32;
  static bool  runDistributed    = true;

  static vec3f up;
  static vec3f pos;
  static vec3f gaze;
  static float fovy = 60.f;
  static bool customView = false;

  // Compute an X x Y x Z grid to have num bricks,
  // only gives a nice grid for numbers with even factors since
  // we don't search for factors of the number, we just try dividing by two
  vec3i computeGrid(int num)
  {
    vec3i grid(1);
    int axis = 0;
    while (num % 2 == 0) {
      grid[axis] *= 2;
      num /= 2;
      axis = (axis + 1) % 3;
    }
    if (num != 1) {
      grid[axis] = num;
    }
    return grid;
  }

  std::pair<ospray::cpp::Geometry, box3f> makeSpheres()
  {
    struct Sphere
    {
      vec3f org;
      int colorID {0};
    };

    std::vector<Sphere> spheres(numSpheresPerNode);

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_real_distribution<float> dist(sceneLowerBound,
                                               sceneUpperBound);

    for (auto &s : spheres) {
      s.org.x = dist(rng);
      s.org.y = dist(rng);
      s.org.z = dist(rng);
    }

    ospray::cpp::Data sphere_data(numSpheresPerNode * sizeof(Sphere),
                                  OSP_UCHAR, spheres.data());

    auto numRanks = static_cast<float>(mpicommon::numGlobalRanks());
    auto myRank   = mpicommon::globalRank();

#if 1
    float r = (numRanks - myRank) / numRanks;
    float b = myRank / numRanks;
    float g = myRank > numRanks / 2 ? 2 * r : 2 * b;
#else
    float normRank = myRank / numRanks;
    float r = 2 * (1.f - normRank - 0.5f);
    if (r < 0.f) r = 0.f;
    float b = 2 * (normRank - 0.5f);
    if (b < 0.f) b = 0.f;
    float g = myRank < numRanks / 2 ? 1.f - r : 1.f - b;
    g *= 0.5f;
#endif

    vec4f color(r, g, b, 1.f);

    ospray::cpp::Data color_data(1, OSP_FLOAT4, &color);

    ospray::cpp::Geometry geom("spheres");
    geom.set("spheres", sphere_data);
    geom.set("color", color_data);
    geom.set("offset_colorID", int(sizeof(vec3f)));
    geom.set("radius", sphereRadius);
    geom.commit();

    //NOTE: all ranks will have the same bounding box, no matter what spheres
    //      were randomly generated
    auto bbox = box3f(vec3f(sceneLowerBound), vec3f(sceneUpperBound));

    return std::make_pair(geom, bbox);
  }

  void setupCamera(ospray::cpp::Camera &camera, box3f worldBounds)
  {
    if (!customView) {
      vec3f diag = worldBounds.size();
      diag       = max(diag,vec3f(0.3f*length(diag)));

      gaze = ospcommon::center(worldBounds);

      pos = gaze - .75f*vec3f(-.6*diag.x,-1.2f*diag.y,.8f*diag.z);
      up  = vec3f(0.f, 1.f, 0.f);
    }

    camera.set("pos", pos);
    camera.set("dir", gaze - pos);
    camera.set("up",  up );
    camera.set("aspect", static_cast<float>(fbSize.x)/fbSize.y);

    camera.commit();
  }

  void parseCommandLine(int ac, const char **av)
  {
    for (int i = 0; i < ac; ++i) {
      std::string arg = av[i];
      if (arg == "-w") {
        fbSize.x = std::atoi(av[++i]);
      } else if (arg == "-h") {
        fbSize.y = std::atoi(av[++i]);
      } else if (arg == "-spn" || arg == "--spheres-per-node") {
        numSpheresPerNode = std::atoi(av[++i]);
      } else if (arg == "-r" || arg == "--radius") {
        sphereRadius = std::atof(av[++i]);
      } else if (arg == "-nf" || arg == "--num-frames") {
        numFrames = std::atoi(av[++i]);
      } else if (arg == "-l" || arg == "--local") {
        runDistributed = false;
      } else if (arg == "-vp" || arg == "--eye") {
        pos.x = atof(av[++i]);
        pos.y = atof(av[++i]);
        pos.z = atof(av[++i]);
        customView = true;
      } else if (arg == "-vu" || arg == "--up") {
        up.x = atof(av[++i]);
        up.y = atof(av[++i]);
        up.z = atof(av[++i]);
        customView = true;
      } else if (arg == "-vi" || arg == "--gaze") {
        gaze.x = atof(av[++i]);
        gaze.y = atof(av[++i]);
        gaze.z = atof(av[++i]);
        customView = true;
      } else if (arg == "-fv" || arg == "--fovy") {
        fovy = atof(av[++i]);
      }
    }
  }

  void initialize_ospray()
  {
    ospLoadModule("ispc");
    ospray::cpp::Device device(nullptr);

    if (runDistributed) {
      ospLoadModule("mpi");
      device = ospray::cpp::Device("mpi_distributed");
      device.set("masterRank", 0);
      device.commit();
      device.setCurrent();
    } else {
      device = ospray::cpp::Device();
      device.commit();
      device.setCurrent();
    }

    ospDeviceSetStatusFunc(device.handle(),
                           [](const char *msg) {
                             std::cerr << msg;
                           });
  }

  void run_benchmark()
  {
    using namespace std::chrono;
    ospray::cpp::Model model;
    auto spheres = makeSpheres();
    model.addGeometry(spheres.first);
    model.commit();

    auto camera = ospray::cpp::Camera("perspective");
    setupCamera(camera, spheres.second);

    ospray::cpp::Renderer renderer;
    if (runDistributed) {
      renderer = ospray::cpp::Renderer("mpi_raycast");
    } else {
      renderer = ospray::cpp::Renderer("raycast");
    }
    renderer.set("world", model);
    renderer.set("model", model);
    renderer.set("camera", camera);
    renderer.set("bgColor", vec3f(0.01f, 0.01f, 0.01f));
    renderer.commit();

    ospray::cpp::FrameBuffer fb(fbSize,OSP_FB_SRGBA,OSP_FB_COLOR|OSP_FB_ACCUM);
    fb.clear(OSP_FB_ACCUM);

    auto bencher = pico_bench::Benchmarker<milliseconds>{numFrames};

    if (runDistributed) {

      if (mpicommon::IamTheMaster()) {
        std::cout << "Benchmark results will be in (ms)" << '\n';
        std::cout << "...starting distributed tests" << '\n';
      }

      mpicommon::world.barrier();

      auto stats = bencher([&](){
        renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);
      });

      if (mpicommon::IamTheMaster()) {
        std::cout << stats << '\n';
        auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
        utility::writePPM("randomSphereTestDistributed.ppm",
                          fbSize.x, fbSize.y, lfb);
        fb.unmap(lfb);
        std::cout << "\noutput: 'randomSphereTestDistributed.ppm'" << std::endl;
      }

      mpicommon::world.barrier();

    } else {

      std::cout << "Benchmark results will be in (ms)" << '\n';
      std::cout << "...starting non-distributed tests" << '\n';

      auto stats = bencher([&](){
        renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);
      });

      std::cout << stats << '\n';

      auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
      utility::writePPM("randomSphereTestLocal.ppm",
                        fbSize.x, fbSize.y, lfb);
      fb.unmap(lfb);
      std::cout << "\noutput: 'randomSphereTestLocal.ppm'" << std::endl;

    }
  }

  extern "C" int main(int ac, const char **av)
  {
    parseCommandLine(ac, av);

    initialize_ospray();
    run_benchmark();

    ospShutdown();
    return 0;
  }

} // ::ospRandSphereTest
