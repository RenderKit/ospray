// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
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

#include <iterator>
#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"

#include "ospcommon/library.h"
#include "ospcommon/FileName.h"
#include "ospcommon/utility/ArrayView.h"
#include "ospcommon/range.h"
#include "ospray_testing.h"

#include <imgui.h>

using namespace ospcommon;

static const std::string renderer_type = "scivis";

// ALOK: AMRVolume class members
std::vector<osp_amr_brick_info> brickInfo;
range1f valueRange;
std::vector<float *> brickPtrs; // ALOK: why????

osp_amr_brick_info bi[4];
float bd[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
                9, 10, 11, 12, 13, 14, 15};
std::vector<float> c = {0, 0, 0,
                        1, 0, 0,
                        0, 1, 0,
                        0, 0, 1};
std::vector<float> o  = {0, 0.333, 0.666, 1.0};

void parseRaw2AmrFile(const FileName &fileName,
                      int BS,
                      int maxLevel)
{
    // ALOK: looks for sibling files created by ospRaw2Amr
    // means that filename arg needs to be base filename without extension
    FileName infoFileName = fileName.str() + std::string(".info");
    FileName dataFileName = fileName.str() + std::string(".data");

    FILE *infoFile = fopen(infoFileName.c_str(), "rb");
    FILE *dataFile = fopen(dataFileName.c_str(), "rb");

    if (infoFile == nullptr) {
        throw std::runtime_error(std::string("#osp:sg - ERROR could not open '")
                + infoFileName.c_str() + "'");
    }

    if (dataFile == nullptr) {
        throw std::runtime_error(std::string("#osp:sg - ERROR could not open '")
                + dataFileName.c_str() + "'");
    }

    osp_amr_brick_info bi;

    auto bounds = box3f(-1, 1);
    auto numCells = BS * BS * BS;

    while (fread(&bi, sizeof(bi), 1, infoFile)) {
        utility::ArrayView<float> bd(new float[numCells], numCells);

        // ALOK: read in actual data from datafile based on the number of cells
        int nr = fread(bd.data(), sizeof(float), numCells, dataFile);
        UNUSED(nr);

        // ALOK: this is ugly way of doing bounds.extend
        // did it this way because there's no copy constructor between f and i
        brickInfo.push_back(bi);
        vec3f boundsf = {bi.bounds.upper.x,
                         bi.bounds.upper.y,
                         bi.bounds.upper.z};
        boundsf += vec3f(1.f);
        bounds.extend(boundsf * bi.cellWidth);
        // ALOK: using osp_amr_brick_info instead
        //bounds.extend((vec3f(bi.box.upper) + vec3f(1.f)) * bi.dt);
        // extend the bounds of the brick(?) based on the brick metadata
        //bounds.extend((vec3f(bi.bounds.upper) + vec3f(1.f)) * bi.cellWidth);

        assert(nr == numCells);
        for (const auto &c : bd)
            valueRange.extend(c); // ALOK: maintaining min/max?
        brickPtrs.push_back(bd);
    }

    // ALOK: setting node children, so these will be parameters of the
    //       AMR volume
    /*
    child("bounds") = bounds;
    child("transferFunction")["valueRange"] = valueRange.toVec2f();
    ospLogF(1) << "read file; found " << brickInfo.size() << " bricks"
        << std::endl;
    */

    fclose(infoFile);
    fclose(dataFile);
}

OSPVolume createDummyAMRVolume()
{
    // ALOK: TODO
    // create a testing volume, e.g. gravity spheres,
    // run raw2amr on it,
    // and return that volume
    OSPVolume dummy = ospNewVolume("amr_volume");
    ospSetString(dummy, "voxelType", "float");

    FileName fname("/mnt/ssd/magnetic_amr");
    parseRaw2AmrFile(fname, 4, 1 << 30);

    /*
    // coarsest level
    osp_vec3i lower = {0, 0, 0}, upper = {3, 3, 3};
    osp_box3i bbox = {lower, upper};
    bi[0].bounds = bbox;
    bi[0].refinementLevel = 0;
    bi[0].cellWidth = 1;
    OSPData brickInfo = ospNewData(1, OSP_VOID_PTR, bi);
    ospCommit(brickInfo);

    OSPData b0 = ospNewData(16, OSP_FLOAT, bd);
    ospCommit(b0);
    OSPData brickData = ospNewData(1, OSP_DATA, b0);
    ospCommit(brickData);

    ospSetData(dummy, "brickInfo", brickInfo);
    ospSetData(dummy, "brickData", brickData);
    */

    OSPData brickInfoData = ospNewData(brickInfo.size(), OSP_VOID_PTR, brickInfo.data());
    ospCommit(brickInfoData);
    OSPData brickDataData = ospNewData(brickPtrs.size(), OSP_FLOAT, brickPtrs.data());
    ospCommit(brickDataData);

    ospSetData(dummy, "brickInfo", brickInfoData);
    ospSetData(dummy, "brickData", brickDataData);

    OSPTransferFunction tf = ospNewTransferFunction("piecewise_linear");
    OSPData colors = ospNewData(4, OSP_FLOAT3, c.data());
    OSPData opacities = ospNewData(4, OSP_FLOAT, o.data());
    ospSetData(tf, "colors", colors);
    ospSetData(tf, "opacities", opacities);
    ospCommit(tf);

    ospSetObject(dummy, "transferFunction", tf);
    ospCommit(dummy);

    return dummy;
}

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    return initError;

  // we must load the testing library explicitly on Windows to look up
  // object creation functions
  loadLibrary("ospray_testing");

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create the world which will contain all of our geometries
  OSPWorld world = ospNewWorld();

  // add in AMR volume
  OSPVolume amrVolume = createDummyAMRVolume();

  // add volume to the world
  ospAddVolume(world, amrVolume);
  box3f bounds(-1, 1);

  // commit the world
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, bounds, world, renderer));

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 64)) {
      ospSet1i(renderer, "spp", spp);
      glfwOSPRayWindow->addObjectToCommit(renderer);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
