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
#include "ospray/ospray_cpp/FrameBuffer.h"
#include "ospray/ospray_cpp/Model.h"
#include "ospray/ospray_cpp/Renderer.h"
#include "ospray/ospray_cpp/TransferFunction.h"
#include "ospray/ospray_cpp/Volume.h"
#include <tfn_lib/tfn_lib.h>
// stl
#include <random>
#include "gensv/generateSciVis.h"

/* This app demonstrates how to write a distributed scivis style
 * batch renderer using the distributed MPI device. Note that because
 * OSPRay uses sort-last compositing it is up to the user to ensure
 * that the data distribution across the nodes is suitable. Specifically,
 * each nodes' data must be convex and disjoint. This renderer only
 * supports multiple volumes and geometries per-node, to ensure they're
 * composited correctly you specify a list of bounding regions to the
 * model, within these regions can be arbitrary volumes/geometries
 * and each rank can have as many regions as needed. As long as the
 * regions are disjoint/convex the data will be rendered correctly.
 * In this example we set two regions on certain ranks just to produce
 * a gap in the ranks volume to demonstrate how they work.
 *
 * In the case that you have geometry crossing the boundary of nodes
 * and are replicating it on both nodes to render (ghost zones, etc.)
 * the region will be used by the renderer to clip rays against allowing
 * to split the object between the two nodes, with each rendering half.
 * This will keep the regions rendered by each rank disjoint and thus
 * avoid any artifacts. For example, if a sphere center is on the border
 * between two nodes, each would render half the sphere and the halves
 * would be composited to produce the final complete sphere in the image.
 *
 * See gensv::loadVolume for an example of how to properly load a volume
 * distributed across ranks with correct specification of brick positions
 * and ghost voxels.
 */

namespace ospDDLoader {

  using namespace ospcommon;

  static std::string volumeFile, dtype;
  static vec3i dimensions        = vec3i(-1);
  static vec2f valueRange        = vec2f(-1);
  static vec2i fbSize            = vec2i(1024, 768);
  static int   numFrames         = 32;
  static int   logLevel          = 0;
  static FileName transferFcn;

  static vec3f up;
  static vec3f pos;
  static vec3f gaze;
  static float fovy = 60.f;
  static bool customView = false;

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
      } else if (arg == "-nf" || arg == "--num-frames") {
        numFrames = std::atoi(av[++i]);
      } else if (arg == "--log") {
        logLevel = std::atoi(av[++i]);
      } else if (arg == "-f") {
        volumeFile = av[++i];
      } else if (arg == "-dtype") {
        dtype = av[++i];
      } else if (arg == "-dims") {
        dimensions.x = std::atoi(av[++i]);
        dimensions.y = std::atoi(av[++i]);
        dimensions.z = std::atoi(av[++i]);
      } else if (arg == "-range") {
        valueRange.x = std::atof(av[++i]);
        valueRange.y = std::atof(av[++i]);
      } else if (arg == "-tfn") {
        transferFcn = FileName(av[++i]);
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

  /* Manually set up the OSPRay device. In MPI distributed mode
   * we use the 'mpi_distributed' renderer, which allows each
   * rank to make separate independent OSPRay calls locally.
   * The model created by this device will handle coordinating
   * the regions of data and the renderer used in the distributed
   * case 'mpi_raycast' knows how to use this information to
   * perform sort-last compositing rendering of the data.
   */
  void initialize_ospray()
  {
    ospLoadModule("ispc");
    ospray::cpp::Device device(nullptr);

    ospLoadModule("mpi");
    device = ospray::cpp::Device("mpi_distributed");
    device.set("masterRank", 0);
    device.set("logLevel", logLevel);
    device.commit();
    device.setCurrent();

    ospDeviceSetStatusFunc(device.handle(),
                           [](const char *msg) {
                             std::cerr << msg;
                           });
  }

  void run_renderer()
  {
    ospray::cpp::Model model;
    gensv::LoadedVolume volume = gensv::loadVolume(volumeFile, dimensions,
                                                   dtype, valueRange);

    if (!transferFcn.str().empty()) {
      tfn::TransferFunction fcn(transferFcn);

      const size_t opacitySamples = 256;
      const float stepSize = 1.0 / opacitySamples;
      std::vector<float> opacities(opacitySamples, 0.f);
      auto iter = fcn.opacityValues.cbegin();
      // Re-sample the opacity values
      for (size_t i = 0; i < opacitySamples; ++i){
        const float x = stepSize * i;
        if (x > (iter + 1)->x) {
          ++iter;
        }
        const float t = (x - iter->x) / ((iter + 1)->x - iter->x);
        opacities[i] = clamp((1.0 - t) * iter->y + t * (iter + 1)->y);
      }

      ospray::cpp::Data colorData(fcn.rgbValues.size(), OSP_FLOAT3, fcn.rgbValues.data());
      ospray::cpp::Data opacityData(opacities.size(), OSP_FLOAT, opacities.data());
      colorData.commit();
      opacityData.commit();

      volume.tfcn.set("colors", colorData);
      volume.tfcn.set("opacities", opacityData);
      volume.tfcn.commit();
    }

    model.addVolume(volume.volume);

    // We must use the global world bounds, not our local bounds
    // when computing the automatically picked camera position.
    box3f worldBounds(vec3f(0), vec3f(dimensions) - vec3f(1));

    /* The regions listing specifies the data regions that this rank owns
     * and is responsible for rendering. All volumes and geometry on the rank
     * should be contained within these bounds and will be clipped against them.
     * In the case of ghost regions or splitting geometry across the region border
     * it's up to the user to ensure the other rank also has the geometry being
     * split and renders the correct region bounds. The region data is specified
     * as an OSPData of OSP_FLOAT3 to pass the lower and upper corners of each
     * regions bounding box.
     */
    std::vector<box3f> regions{volume.bounds};
    ospray::cpp::Data regionData(regions.size() * 2, OSP_FLOAT3,
        regions.data());
    model.set("regions", regionData);
    model.commit();

    auto camera = ospray::cpp::Camera("perspective");
    setupCamera(camera, worldBounds);

    // In the distributed mode we use the 'mpi_raycast' renderer which
    // knows how to read the region information from the model and render
    // the distributed data.
    ospray::cpp::Renderer renderer = ospray::cpp::Renderer("mpi_raycast");
    renderer.set("world", model);
    renderer.set("model", model);
    renderer.set("camera", camera);
    renderer.set("bgColor", vec3f(0.02));
    renderer.commit();

    ospray::cpp::FrameBuffer fb(fbSize,OSP_FB_SRGBA,OSP_FB_COLOR|OSP_FB_ACCUM);
    fb.clear(OSP_FB_ACCUM);


    mpicommon::world.barrier();

    auto frameStartTime = ospcommon::getSysTime();

    for (int i = 0; i < numFrames; ++i) {
      if (mpicommon::IamTheMaster())
        std::cout << "rendering frame " << i << std::endl;

      renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);
    }

    double seconds = ospcommon::getSysTime() - frameStartTime;

    // Only the OSPRay master rank will have the final framebuffer which
    // can be saved out or displayed to the user, the others only store
    // the tiles which they composite.
    if (mpicommon::IamTheMaster()) {
      auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
      utility::writePPM("ddLoaderTest.ppm", fbSize.x, fbSize.y, lfb);
      fb.unmap(lfb);
      std::cout << "\noutput: 'ddLoaderTest.ppm'" << std::endl;
      std::cout << "\nrendered " << numFrames << " frames at an avg rate of "
        << numFrames / seconds << " frames per second" << std::endl;
    }
  }

  extern "C" int main(int ac, const char **av)
  {
    parseCommandLine(ac, av);
    if (volumeFile.empty()) {
      std::cerr << "Error: -f <volume file.raw> is required\n";
      return 1;
    }
    if (dtype.empty()) {
      std::cerr << "Error: -dtype (uchar|char|float|double) is required\n";
      return 1;
    }
    if (dimensions == vec3i(-1)) {
      std::cerr << "Error: -dims X Y Z is required to pass volume dims\n";
      return 1;
    }
    if (valueRange == vec2f(-1)) {
      std::cerr << "Error: -range X Y is required to set transfer function range\n";
      return 1;
    }

    initialize_ospray();

    ospShutdown();
    return 0;
  }

}

