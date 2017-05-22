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
#include "ospray/ospray_cpp/TransferFunction.h"
#include "ospray/ospray_cpp/Volume.h"
// ospray apps
#include "common/commandline/CameraParser.h"
#include "widgets/imguiViewer.h"
// stl
#include <random>

#define RUN_LOCAL 0

/* This app demonstrates how to write a distributed scivis style
 * renderer using the distributed MPI device. Note that because
 * OSPRay uses sort-last compositing it is up to the user to ensure
 * that the data distribution across the nodes is suitable. Specifically,
 * each nodes data must be convex, and disjoint. This renderer only
 * supports a single volume per node but within the volume could be
 * arbitrary geometry.
 *
 * In the case that you have geometry crossing the boundary of nodes
 * and are replicating it on both nodes to render (ghost zones, etc.)
 * you can specify the region for the renderer to clip rays against
 * its data region, keeping the regions disjoint. For example, if a
 * sphere center is on the border between two nodes, each would
 * render half the sphere and the halves would be composited to produce
 * the final whole-sphere in the image.
 */

namespace ospRandSciVisTest {

  using namespace ospcommon;

  int   numSpheresPerNode = 100;
  float sphereRadius      = 0.01f;
  vec2i fbSize            = vec2i(1024, 768);
  int   numFrames         = 32;
  bool  runDistributed    = true;
  int   logLevel          = 0;

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

  ospray::cpp::Geometry makeSpheres(const box3f &bbox)
  {
    struct Sphere
    {
      vec3f org;
      int colorID {0};
    };

    auto numRanks = static_cast<float>(mpicommon::numGlobalRanks());
    auto myRank   = mpicommon::globalRank();

    std::vector<Sphere> spheres(numSpheresPerNode);

    std::mt19937 rng;
    rng.seed(std::random_device()());

    // Generate spheres within this nodes volume, to keep the data disjoint.
    // We also leave some buffer space on the boundaries to avoid clipping
    // artifacts or needing duplication across nodes in the case a sphere
    // crosses a boundary. Note: Since we don't communicated ghost regions
    // among the nodes, we make sure not to generate any spheres which would
    // be clipped.
    std::uniform_real_distribution<float> dist_x(bbox.lower.x + sphereRadius,
                                                 bbox.upper.x - sphereRadius);
    std::uniform_real_distribution<float> dist_y(bbox.lower.y + sphereRadius,
                                                 bbox.upper.y - sphereRadius);
    std::uniform_real_distribution<float> dist_z(bbox.lower.z + sphereRadius,
                                                 bbox.upper.z - sphereRadius);

    for (auto &s : spheres) {
      s.org.x = dist_x(rng);
      s.org.y = dist_y(rng);
      s.org.z = dist_z(rng);
    }

    ospray::cpp::Data sphere_data(numSpheresPerNode * sizeof(Sphere),
                                  OSP_UCHAR, spheres.data());


    float r = (numRanks - myRank) / numRanks;
    float b = myRank / numRanks;
    float g = myRank > numRanks / 2 ? 2 * r : 2 * b;
    vec4f color(r, g, b, 1.f);

    ospray::cpp::Data color_data(1, OSP_FLOAT4, &color);

    ospray::cpp::Geometry geom("spheres");
    geom.set("spheres", sphere_data);
    geom.set("color", color_data);
    geom.set("offset_colorID", int(sizeof(vec3f)));
    geom.set("radius", sphereRadius);
    geom.commit();

    return geom;
  }

  std::pair<ospray::cpp::Volume, box3f> makeVolume()
  {
    auto numRanks = static_cast<float>(mpicommon::numGlobalRanks());
    auto myRank   = mpicommon::globalRank();

    ospray::cpp::TransferFunction transferFcn("piecewise_linear");
    const std::vector<vec3f> colors = {
      vec3f(0, 0, 0.56),
      vec3f(0, 0, 1),
      vec3f(0, 1, 1),
      vec3f(0.5, 1, 0.5),
      vec3f(1, 1, 0),
      vec3f(1, 0, 0),
      vec3f(0.5, 0, 0)
    };
    const std::vector<float> opacities = {0.015, 0.015};
    ospray::cpp::Data colorsData(colors.size(), OSP_FLOAT3, colors.data());
    ospray::cpp::Data opacityData(opacities.size(), OSP_FLOAT, opacities.data());
    colorsData.commit();
    opacityData.commit();

    const vec2f valueRange(static_cast<float>(0), static_cast<float>(numRanks));
    transferFcn.set("colors", colorsData);
    transferFcn.set("opacities", opacityData);
    transferFcn.set("valueRange", valueRange);
    transferFcn.commit();

    const vec3i volumeDims(128);
    const vec3i grid = computeGrid(numRanks);
    ospray::cpp::Volume volume("block_bricked_volume");
    volume.set("voxelType", "uchar");
    volume.set("dimensions", volumeDims);
    volume.set("transferFunction", transferFcn);

    const vec3f gridSpacing = vec3f(1.f) / (vec3f(grid) * vec3f(volumeDims));
    volume.set("gridSpacing", gridSpacing);

    const vec3i brickId(myRank % grid.x, (myRank / grid.x) % grid.y, myRank / (grid.x * grid.y));
    const vec3f gridOrigin = vec3f(brickId) * gridSpacing * vec3f(volumeDims);
    volume.set("gridOrigin", gridOrigin);

    std::vector<unsigned char> volumeData(volumeDims.x * volumeDims.y * volumeDims.z, 0);
    for (size_t i = 0; i < volumeData.size(); ++i) {
      volumeData[i] = myRank;
    }
    volume.setRegion(volumeData.data(), vec3i(0), volumeDims);
    volume.commit();

    auto bbox = box3f(gridOrigin, gridOrigin + vec3f(1.f) / vec3f(grid));
    return std::make_pair(volume, bbox);
  }

  void setupCamera(ospray::cpp::Camera &camera, box3f worldBounds)
  {
    vec3f center = ospcommon::center(worldBounds);
    vec3f diag   = worldBounds.size();
    diag         = max(diag,vec3f(0.3f*length(diag)));
    vec3f from   = center - .85f*vec3f(-.6*diag.x,-1.2f*diag.y,.8f*diag.z);
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
      } else if (arg == "--log") {
        logLevel = std::atoi(av[++i]);
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
      device.set("logLevel", logLevel);
      device.commit();
      device.setCurrent();
    } else {
      device = ospray::cpp::Device();
      device.set("logLevel", logLevel);
      device.commit();
      device.setCurrent();
    }

    ospDeviceSetStatusFunc(device.handle(),
                           [](const char *msg) {
                             std::cerr << msg;
                           });
  }

  extern "C" int main(int ac, const char **av)
  {
    parseCommandLine(ac, av);

    initialize_ospray();

    ospray::cpp::Model model;
    auto volume = makeVolume();
    model.addVolume(volume.first);

    // Generate spheres within the bounds of the volume
    auto spheres = makeSpheres(volume.second);
    model.addGeometry(spheres);

    // Note: now we must use the global world bounds, not our local bounds
    box3f worldBounds(vec3f(0), vec3f(1));

    // On some ranks we add some additional regions to clip the volume
    // and make some gaps, just to show usage and test multiple regions per-rank
    std::vector<box3f> regions{volume.second};
    bool setGap = false;
    if (mpicommon::numGlobalRanks() % 2 == 0) {
      setGap = mpicommon::globalRank() % 3 == 0;
    } else  {
      setGap = mpicommon::globalRank() % 2 == 0;
    }
    if (setGap) {
      const float step = (regions[0].upper.x - regions[0].lower.x) / 4.0;
      const vec3f low = regions[0].lower;
      const vec3f hi = regions[0].upper;
      regions[0].upper.x = low.x + step;
      regions.push_back(box3f(vec3f(low.x + step * 3, low.y, low.z),
                                vec3f(low.x + step * 4, hi.y, hi.z)));
    }
    ospray::cpp::Data regionData(regions.size() * 2, OSP_FLOAT3,
        regions.data());
    model.set("regions", regionData);
    model.commit();

    DefaultCameraParser cameraClParser;
    cameraClParser.parse(ac, av);
    auto camera = cameraClParser.camera();
    setupCamera(camera, worldBounds);

    ospray::cpp::Renderer renderer;
    if (runDistributed) {
      renderer = ospray::cpp::Renderer("mpi_raycast");
    } else {
      renderer = ospray::cpp::Renderer("raycast");
    }
    renderer.set("world", model);
    renderer.set("model", model);
    renderer.set("camera", camera);
    renderer.set("bgColor", vec3f(0.02));
    renderer.commit();

    ospray::cpp::FrameBuffer fb(fbSize,OSP_FB_SRGBA,OSP_FB_COLOR|OSP_FB_ACCUM);
    fb.clear(OSP_FB_ACCUM);

    if (runDistributed) {

      mpicommon::world.barrier();

      auto frameStartTime = ospcommon::getSysTime();

      for (int i = 0; i < numFrames; ++i) {
        if (mpicommon::IamTheMaster())
          std::cout << "rendering frame " << i << std::endl;

        renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);
      }

      double seconds = ospcommon::getSysTime() - frameStartTime;

      if (mpicommon::IamTheMaster()) {
        auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
        writePPM("randomSciVisTestDistributed.ppm", fbSize.x, fbSize.y, lfb);
        fb.unmap(lfb);
        std::cout << "\noutput: 'randomSciVisTestDistributed.ppm'" << std::endl;
        std::cout << "\nrendered " << numFrames << " frames at an avg rate of "
                  << numFrames / seconds << " frames per second" << std::endl;
      }

      mpicommon::world.barrier();

    } else {

      auto frameStartTime = ospcommon::getSysTime();

      for (int i = 0; i < numFrames; ++i) {
        std::cout << "rendering frame " << i << std::endl;

        renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);
      }

      double seconds = ospcommon::getSysTime() - frameStartTime;

      auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
      writePPM("randomSciVisTestLocal.ppm", fbSize.x, fbSize.y, lfb);
      fb.unmap(lfb);
      std::cout << "\noutput: 'randomSciVisTestLocal.ppm'" << std::endl;
      std::cout << "\nrendered " << numFrames << " frames at an avg rate of "
                << numFrames / seconds << " frames per second" << std::endl;

    }

    return 0;
  }

}

