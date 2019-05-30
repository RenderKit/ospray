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

// ALOK: AMRVolume context info
std::vector<osp_amr_brick_info> brickInfo; // holds brick metadata
std::vector<float *> brickPtrs;            // holds actual data
std::vector<OSPData> brickData;            // holds actual data as OSPData
range1f valueRange;

std::vector<float> c = {0, 0, 0,
                        1, 0, 0,
                        0, 1, 0,
                        0, 0, 1};
std::vector<float> o  = {0, 0.333, 0.666, 1.0};

// ALOK: TODO
// this is ugly
// somehow we're using vec/box types that don't have size() or
// product(). that needs to change!
int numCellsInBrick(const osp_amr_brick_info &bi)
{
    vec3i size = {bi.bounds.upper.x - bi.bounds.lower.x + 1,
                  bi.bounds.upper.y - bi.bounds.lower.y + 1,
                  bi.bounds.upper.z - bi.bounds.lower.z + 1};
    int product = size.x * size.y * size.z;
    return product;
}

std::ostream &operator<<(std::ostream &os, const osp_vec3i &v)
{
    os << "<" << v.x << ", " <<
                 v.y << ", " <<
                 v.z << ">";
    return os;
}

std::ostream &operator<<(std::ostream &os, const osp_amr_brick_info &bi)
{
    os << "    ________" << std::endl
       << "   /\\       \\      bounds lower" << std::endl
       << "  /::\\       \\      " << bi.bounds.lower << std::endl
       << " /::::\\       \\    bounds upper" << std::endl
       << " \\:::::\\       \\    " << bi.bounds.upper << std::endl
       << "  \\:::::\\       \\  refinementLevel" << std::endl
       << "   \\:::::\\_______\\  " << bi.refinementLevel << std::endl
       << "    \\::::/ZZZZZZZ/ cellWidth" << std::endl
       << "     \\::/ZZZZZZZ/   " << bi.cellWidth << std::endl
       << "      \\/ZZZZZZZ/" << std::endl;
    return os;
}

void parseRaw2AmrFile(const FileName &fileName,
                      int BS,
                      int maxLevel)
{
    // filename arg needs to be base filename without extension
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

    // populate brickInfo and brickPtrs
    // by reading in osp_amr_brick_info structs from infoFile
    // and reading corresponding data from dataFile
    while (fread(&bi, sizeof(bi), 1, infoFile)) {
        if(bi.refinementLevel > maxLevel) {
            continue;
        }

        std::cout << bi << std::endl;

        utility::ArrayView<float> bd(new float[numCells], numCells);

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

        assert(nr == numCells);
        for (const auto &c : bd)
            valueRange.extend(c);

        brickPtrs.push_back(bd);
    }

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

    FileName fname("/mnt/ssd/test_data/test_amr");
    parseRaw2AmrFile(fname, 4, 0);

    // ALOK: taken from ospray/master's AMRVolume::preCommit()
    // convert raw pointers in brickPtrs to OSPData in brickData
    for(size_t bID = 0; bID < brickInfo.size(); bID++) {
        const osp_amr_brick_info &bi = brickInfo[bID];
        int numCells = numCellsInBrick(bi);
        OSPData data = ospNewData(numCells, OSP_FLOAT, brickPtrs[bID], OSP_DATA_SHARED_BUFFER);
        brickData.push_back(data);
    }

    OSPData brickInfoData = ospNewData(brickInfo.size(), OSP_VOID_PTR, brickInfo.data());
    ospCommit(brickInfoData);

    OSPData brickDataData = ospNewData(brickData.size(), OSP_DATA, brickData.data());
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
