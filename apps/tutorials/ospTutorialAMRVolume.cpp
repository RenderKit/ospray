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
float *actualData;

// ALOK: TODO
// this is ugly
// somehow we're using vec/box types that don't have size() or
// product(). that needs to change!
int numCellsInBrick(const osp_amr_brick_info &bi)
{
    vec3i size = {bi.bounds[0] - bi.bounds[3] + 1,
                  bi.bounds[1] - bi.bounds[4] + 1,
                  bi.bounds[2] - bi.bounds[5] + 1};
    int product = size.x * size.y * size.z;
    return product;
}

std::ostream &operator<<(std::ostream &os, const vec3i &v)
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
       << "  /::\\       \\      " << bi.bounds[0] << " " <<  bi.bounds[1] << " " << bi.bounds[2] << " " << std::endl
       << " /::::\\       \\    bounds upper" << std::endl
       << " \\:::::\\       \\    " << bi.bounds[3] << " " <<  bi.bounds[4] << " " << bi.bounds[5] << " " << std::endl
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
            // ALOK: need to move the data file pointer forward
            // so that we're reading the correct data from next
            // block
            fseek(dataFile, numCells * sizeof(float), SEEK_CUR);
            continue;
        }

        //std::cout << bi << std::endl;

        utility::ArrayView<float> bd(new float[numCells], numCells);

        int nr = fread(bd.data(), sizeof(float), numCells, dataFile);
        UNUSED(nr);

        // ALOK: this is ugly way of doing bounds.extend
        // did it this way because there's no copy constructor between f and i
        brickInfo.push_back(bi);
        vec3f boundsf = {bi.bounds[0],
                         bi.bounds[1],
                         bi.bounds[2]};
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

    //FileName fname("/mnt/ssd/test_data/test_amr");
    FileName fname("/mnt/ssd/magnetic_amr");
    parseRaw2AmrFile(fname, 4, 3);
    
    // ALOK: taken from ospray/master's AMRVolume::preCommit()
    // convert raw pointers in brickPtrs to OSPData in brickData
    for(size_t bID = 0; bID < brickInfo.size(); bID++) {
        const osp_amr_brick_info &bi = brickInfo[bID];
        int numCells = numCellsInBrick(bi);
        OSPData data = ospNewData(numCells, OSP_FLOAT, brickPtrs[bID], OSP_DATA_SHARED_BUFFER);
        brickData.push_back(data);
    }

    OSPData brickInfoData = ospNewData(brickInfo.size()*sizeof(osp_amr_brick_info), OSP_RAW, brickInfo.data());
    ospCommit(brickInfoData);

    OSPData brickDataData = ospNewData(brickData.size(), OSP_DATA, brickData.data(), OSP_DATA_SHARED_BUFFER);
    ospCommit(brickDataData);

    ospSetData(dummy, "brickInfo", brickInfoData);
    ospSetData(dummy, "brickData", brickDataData);

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

  // create a volume instance container
  OSPVolumeInstance instance = ospNewVolumeInstance(amrVolume);

  // apply a transfer function to the instance
  osp_vec2f amrValueRange = {valueRange.lower, valueRange.upper};
  OSPTransferFunction tf = ospTestingNewTransferFunction(amrValueRange, "jet");
  ospSetObject(instance, "transferFunction", tf);
  ospCommit(instance);

  // create a data array of all instances
  OSPData volumeInstances = ospNewData(1, OSP_OBJECT, &instance);

  // add volume instance to the world
  ospSetObject(world, "volumes", volumeInstances);
  box3f bounds(0, 64);

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

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
