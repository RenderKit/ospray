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

#include <string>
#include "llnlrm_reader.h"

using namespace gensv;
using namespace ospcommon;

LLNLRMReader::LLNLRMReader(const ospcommon::FileName &bobDir)
  : bobDir(bobDir)
{
  std::string s = bobDir.base();
  s = s.substr(3);
  timestep = std::stoi(s);
}
void LLNLRMReader::loadBlock(const size_t blockID, containers::AlignedVector<char> &data) const {
  if (blockID >= numBlocks()) {
    throw std::runtime_error("Invalid blockID for LLNL RM Data");
  }

  // The RM Bob filenames are at most 15 characters long,
  // in the case of gzipped ones
  char bobName[16] = {0};
  FILE *file = NULL;
  bool useGZip = true;
  if (useGZip) {
#ifndef _WIN32
    if (std::snprintf(bobName, 16, "d_%04d_%04li.gz", timestep, blockID) > 15) {
      throw std::runtime_error("sg::RichtmyerMeshkov: Invalid timestep or blockID!");
    }
    const FileName fileName = bobDir + FileName(bobName);
    const std::string cmd = "gunzip -c " + fileName.str();
    file = popen(cmd.c_str(), "r");
    if (!file) {
      throw std::runtime_error("sg::RichtmyerMeshkov: could not open popen '"
          + cmd + "'");
    }
#else
    throw std::runtime_error("sg::RichtmyerMeshkov: gzipped RM bob's"
        " aren't supported on Windows!");
#endif
  } else {
    if (std::snprintf(bobName, 16, "d_%04d_%04li", timestep, blockID) > 15) {
      throw std::runtime_error("sg::RichtmyerMeshkov: Invalid timestep or blockID!");
    }
    const FileName fileName = bobDir + FileName(bobName);
    file = fopen(fileName.c_str(), "rb");
    if (!file) {
      throw std::runtime_error("sg::RichtmyerMeshkov: could not open file '"
          + std::string(fileName) + "'");
    }
  }

  const vec3sz blockDims = blockSize();
  const size_t blockBytes = blockDims.x * blockDims.y * blockDims.z;
  if (data.size() < blockBytes) {
    data.resize(blockBytes);
  }

  if (std::fread(data.data(), blockBytes, 1, file) != 1) {
    if (useGZip) {
#ifndef _WIN32
      pclose(file);
#endif
    } else {
      fclose(file);
    }
    throw std::runtime_error("sg::RichtmyerMeshkov: failed to read data from bob "
        + std::string(bobName));
  }

  if (useGZip) {
#ifndef _WIN32
    pclose(file);
#endif
  } else {
    fclose(file);
  }
}
size_t LLNLRMReader::numBlocks() {
  return 8 * 8 * 15;
}
vec3sz LLNLRMReader::blockGrid() {
  return vec3sz(8, 8, 15);
}
vec3sz LLNLRMReader::blockSize() {
  return vec3sz(256, 256, 128);
}
vec3sz LLNLRMReader::dimensions() {
  return vec3sz(2048, 2048, 1920);
}
/*

   void RichtmyerMeshkov::loaderThread(LoaderState &state)
   {
   Node &progressLog = child("blocksLoaded");
   std::vector<uint8_t> block(LoaderState::BLOCK_SIZE, 0);
   while (true) {
   const size_t blockID = state.loadNextBlock(block);
   if (blockID >= LoaderState::NUM_BLOCKS) {
   break;
   }

   const int I = blockID % 8;
   const int J = (blockID / 8) % 8;
   const int K = (blockID / 64);

   vec2f blockRange(block[0]);
   extendVoxelRange(blockRange, &block[0], LoaderState::BLOCK_SIZE);
   const vec3i region_lo(I * 256, J * 256, K * 128);
   const vec3i region_sz(256, 256, 128);
   {
   std::lock_guard<std::mutex> lock(state.mutex);
   ospSetRegion(volume, block.data(), (const osp::vec3i&)region_lo,
   (const osp::vec3i&)region_sz);

   state.voxelRange.x = std::min(state.voxelRange.x, blockRange.x);
   state.voxelRange.y = std::max(state.voxelRange.y, blockRange.y);
   progressLog.setValue(std::to_string(blockID + 1) + "/"
   + std::to_string(LoaderState::NUM_BLOCKS));
   }
   */

