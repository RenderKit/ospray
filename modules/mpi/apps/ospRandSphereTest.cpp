// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// mpiCommon
#include "mpiCommon/MPICommon.h"
// public-ospray
#include "ospray/ospray_cpp/Camera.h"
#include "ospray/ospray_cpp/Data.h"
#include "ospray/ospray_cpp/Device.h"
#include "ospray/ospray_cpp/FrameBuffer.h"
#include "ospray/ospray_cpp/Renderer.h"
// ospray apps
#include "common/commandline/CameraParser.h"
#include "widgets/imguiViewer.h"
// stl
#include <random>

#define RUN_LOCAL 0

namespace ospRandSphereTest {

  using namespace ospcommon;

  int   numSpheresPerNode = 100;
  float sphereRadius      = 0.01f;
  float sceneLowerBound   = 0.f;
  float sceneUpperBound   = 1.f;
  vec2i fbSize            = vec2i(1024, 768);
  int   numFrames         = 32;
  bool  runDistributed    = true;

  //TODO: factor this into a reusable piece inside of ospcommon!!!!!!
  // helper function to write the rendered image as PPM file
  void writePPM(const std::string &fileName,
                const int sizeX, const int sizeY,
                const uint32_t *pixel)
  {
    FILE *file = fopen(fileName.c_str(), "wb");
    fprintf(file, "P6\n%i %i\n255\n", sizeX, sizeY);
    unsigned char *out = (unsigned char *)alloca(3*sizeX);
    for (int y = 0; y < sizeY; y++) {
      auto *in = (const unsigned char *)&pixel[(sizeY-1-y)*sizeX];
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

  std::pair<ospray::cpp::Model, box3f> makeSpheres()
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

    auto numRanks = static_cast<float>(ospray::mpi::numGlobalRanks());
    auto myRank   = ospray::mpi::globalRank();

    vec4f color((numRanks - myRank) / numRanks, 0.f, myRank / numRanks, 1.f);

    ospray::cpp::Data color_data(1, OSP_FLOAT4, &color);

    ospray::cpp::Geometry geom("spheres");
    geom.set("spheres", sphere_data);
    geom.set("color", color_data);
    geom.set("offset_colorID", int(sizeof(vec3f)));
    geom.set("radius", sphereRadius);
    geom.commit();

    ospray::cpp::Model model;
    model.addGeometry(geom);
    model.commit();

    //NOTE: all ranks will have the same bounding box, no matter what spheres
    //      were randomly generated
    auto bbox = box3f(vec3f(sceneLowerBound), vec3f(sceneUpperBound));

    return std::make_pair(model, bbox);
  }

  void setupCamera(ospray::cpp::Camera &camera, box3f worldBounds)
  {
    vec3f center = ospcommon::center(worldBounds);
    vec3f diag   = worldBounds.size();
    diag         = max(diag,vec3f(0.3f*length(diag)));
    vec3f from   = center - .75f*vec3f(-.6*diag.x,-1.2f*diag.y,.8f*diag.z);
    vec3f dir    = center - from;

    camera.set("pos", from);
    camera.set("dir", dir);
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
      }
    }
  }

  void initialize_ospray()
  {
    ospray::cpp::Device device;

    if (runDistributed) {
      ospLoadModule("mpi");
      device = ospray::cpp::Device("mpi_distributed");
      device.set("masterRank", 0);
      device.commit();
      device.setCurrent();
    } else {
      device = ospray::cpp::Device("default");
      device.commit();
      device.setCurrent();
    }

    ospDeviceSetErrorMsgFunc(device.handle(),
                             [](const char *msg) {
                               std::cerr << msg << std::endl;
                             });
  }

  extern "C" int main(int ac, const char **av)
  {
    parseCommandLine(ac, av);

    initialize_ospray();

    auto scene = makeSpheres();

    DefaultCameraParser cameraClParser;
    cameraClParser.parse(ac, av);
    auto camera = cameraClParser.camera();
    setupCamera(camera, scene.second);

    ospray::cpp::Renderer renderer("raycast");
    renderer.set("world", scene.first);
    renderer.set("model", scene.first);
    renderer.set("camera", camera);
    renderer.set("bgColor", vec3f(1.f, 1.f, 1.f));
    renderer.commit();

    ospray::cpp::FrameBuffer fb(fbSize,OSP_FB_SRGBA,OSP_FB_COLOR|OSP_FB_ACCUM);
    fb.clear(OSP_FB_ACCUM);

    if (runDistributed) {
      ospray::mpi::world.barrier();

      auto frameStartTime = ospcommon::getSysTime();

      for (int i = 0; i < numFrames; ++i) {
        if (ospray::mpi::IamTheMaster())
          std::cout << "rendering frame " << i << std::endl;

        renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);

        ospray::mpi::world.barrier();
      }

      double seconds = ospcommon::getSysTime() - frameStartTime;

      if (ospray::mpi::IamTheMaster()) {
        auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
        writePPM("randomSphereTestDistributed.ppm", fbSize.x, fbSize.y, lfb);
        fb.unmap(lfb);
        std::cout << "\noutput: 'randomSphereTestDistributed.ppm'" << std::endl;
        std::cout << "\nrendered " << numFrames << " frames at an avg rate of "
                  << numFrames / seconds << " frames per second" << std::endl;
      }

      ospray::mpi::world.barrier();
    } else {
      renderer.renderFrame(fb, OSP_FB_COLOR);

      auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
      writePPM("randomSphereTestLocal.ppm", fbSize.x, fbSize.y, lfb);
      fb.unmap(lfb);
      std::cout << "output: 'randomSphereTestLocal.ppm'" << std::endl;
    }

    return 0;
  }

} // ::ospRandSphereTest
