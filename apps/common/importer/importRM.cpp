// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

// own
#include "Importer.h"
// ospcommon
#include "ospcommon/FileName.h"
#include "ospcommon/sysinfo.h"
// ospray api
#include "ospray/ospray.h"

// std::
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>

namespace ospray {
  namespace importer {
    struct RMLoaderThreads {
      Volume *volume;

      std::mutex mutex;
      std::atomic<int> nextBlockID;
      std::atomic<int> nextPinID;
      int numThreads;
      int timeStep;
      std::vector<std::thread> threads;
      std::string inFilesDir;
      bool useGZip;

      struct Block {
        uint8_t voxel[256*256*128];
      };

      RMLoaderThreads(Volume *volume, const std::string &fileName, int numThreads=10)
        : volume(volume), nextBlockID(0), nextPinID(0), numThreads(numThreads)
      {
        inFilesDir = fileName.substr(0, fileName.rfind('.'));
        std::cout << "Reading LLNL Richtmyer-Meshkov bob from " << inFilesDir
          << " with " << numThreads << " threads" << std::endl;

        useGZip = (getenv("OSPRAY_RM_NO_GZIP") == NULL);

        const size_t slash = fileName.rfind('/');
        std::string base = slash != std::string::npos ? fileName.substr(slash + 1) : fileName;

        int rc = sscanf(base.c_str(),"bob%03d.bob",&timeStep);
        if (rc != 1)
          throw std::runtime_error("could not extract time step from bob file name "+base);

        volume->voxelRange.x = +std::numeric_limits<float>::infinity();
        volume->voxelRange.y = -std::numeric_limits<float>::infinity();

        threads.reserve(numThreads);
        for (int i = 0; i < numThreads; ++i) {
          threads.push_back(std::thread([&](){ run(); }));
        }
        for (size_t i = 0; i < threads.size(); ++i) {
          threads[i].join();
        }
      };

      void loadBlock(Block &block, const std::string &fileNameBase, size_t blockID){
        char fileName[10000];
        FILE *file;
        if (useGZip) {
#ifndef _WIN32
          sprintf(fileName,"%s/d_%04d_%04li.gz",
              fileNameBase.c_str(),timeStep,blockID);
          const std::string cmd = "gunzip -c "+std::string(fileName);
          file = popen(cmd.c_str(),"r");
          if (!file)
            throw std::runtime_error("could not open file in popen command '"+cmd+"'");
#else
          throw std::runtime_error("gzipped RM bob's aren't supported on Windows!");
#endif
        } else {
          sprintf(fileName,"%s/d_%04d_%04li",
              fileNameBase.c_str(),timeStep,blockID);
          file = fopen(fileName,"rb");
          if (!file)
            throw std::runtime_error("could not open '"+std::string(fileName)+"'");
        }

        assert(file);
        int rc = fread(block.voxel,sizeof(uint8_t),256*256*128,file);
        if (rc != 256*256*128) {
          PRINT(rc);
          PRINT(256*256*128);
          throw std::runtime_error("could not read enough data from "+std::string(fileName));
        }

        if (useGZip) {
#ifndef _WIN32
          pclose(file);
#else
          throw std::runtime_error("gzipped RM bob's aren't supported on Windows!");
#endif
        } else {
          fclose(file);
        }
      }

      void run()
      {
        int threadID = nextPinID.fetch_add(1);

        Block *block = new Block;
        while(1) {
          int blockID = nextBlockID.fetch_add(1);
          if (blockID >= 8*8*15) break;

          // int b = K*64+J*8+I;
          int I = blockID % 8;
          int J = (blockID / 8) % 8;
          int K = (blockID / 64);


#if OSPRAY_APPS_IMPORTER_ENABLE_PRINTS
          printf("[b%i:%i,%i,%i,(%i)]",blockID,I,J,K,threadID);
#endif
          loadBlock(*block,inFilesDir,blockID);

          ospcommon::vec2f blockRange(block->voxel[0]);
          extendVoxelRange(blockRange,&block->voxel[0],256*256*128);
          ospcommon::vec3i region_lo(I*256,J*256,K*128);
          ospcommon::vec3i region_sz(256,256,128);
          {
            std::lock_guard<std::mutex> lock(mutex);
            ospSetRegion(volume->handle,block->voxel,(osp::vec3i&)region_lo,(osp::vec3i&)region_sz);

            volume->voxelRange.x = std::min(volume->voxelRange.x,blockRange.x);
            volume->voxelRange.y = std::max(volume->voxelRange.y,blockRange.y);
          }
        }
        delete block;
      }
    };

    // Just import the RM file into some larger scene, just loads the volume
    void importVolumeRM(const FileName &fileName, Volume *volume) {
      const char *scaleFactorEnv = getenv("OSPRAY_VOLUME_SCALE_FACTOR");
      if (scaleFactorEnv){
        std::cout << "#importRM: found OSPRAY_VOLUME_SCALE_FACTOR env-var\n";
        vec3f scaleFactor;
        if (sscanf(scaleFactorEnv, "%fx%fx%f", &scaleFactor.x, &scaleFactor.y, &scaleFactor.z) != 3){
          throw std::runtime_error("Could not parse OSPRAY_RM_SCALE_FACTOR env-var. Must be of format"
              "<X>x<Y>x<Z> (e.g '1.5x2x0.5')");
        }
        std::cout << "#importRM: got OSPRAY_VOLUME_SCALE_FACTOR env-var = {"
          << scaleFactor.x << ", " << scaleFactor.y << ", " << scaleFactor.z
          << "}\n";
        volume->scaleFactor = scaleFactor;
        ospSetVec3f(volume->handle, "scaleFactor", (osp::vec3f&)volume->scaleFactor);
      }

      // Update the provided dimensions of the volume for the subvolume specified.
      ospcommon::vec3i dims(2048,2048,1920);
      volume->dimensions = dims;
      if (volume->scaleFactor != vec3f(1.f)) {
        dims = vec3i(vec3f(dims) * volume->scaleFactor);
      }
      ospSetVec3i(volume->handle, "dimensions", (osp::vec3i&)dims);
      ospSetString(volume->handle,"voxelType", "uchar");

      // Note: API is not thread safe, even w/ the locking around the ospSetRegion call
      // the MPI backend has issues.
      const int numThreads = 1;

      double t0 = ospcommon::getSysTime();

      RMLoaderThreads(volume,fileName,numThreads);
      double t1 = ospcommon::getSysTime();
      std::cout << "done loading " << fileName
        << ", needed " << (t1-t0) << " seconds" << std::endl;

      ospSet2f(volume->handle,"voxelRange",volume->voxelRange.x,volume->voxelRange.y);
      volume->dimensions = dims;
      volume->bounds = ospcommon::empty;
      volume->bounds.extend(volume->gridOrigin);
      volume->bounds.extend(volume->gridOrigin+ vec3f(volume->dimensions) * volume->gridSpacing);
    }

    // Treat a single RM file as a "scene"
    void importRM(const FileName &fileName, Group *group) {
      const char *dpFromEnv = getenv("OSPRAY_DATA_PARALLEL");
      // Same as OSPObjectFile::importVolume for creating the initial OSPVolume and importRAW
      Volume *volume = new Volume;
      if (dpFromEnv) {
        // Create the OSPRay object.
        std::cout << "#osp.loader: found OSPRAY_DATA_PARALLEL env-var, "
          << "#osp.loader: trying to use data _parallel_ mode..." << std::endl;
        osp::vec3i blockDims;
        int rc = sscanf(dpFromEnv,"%dx%dx%d",&blockDims.x,&blockDims.y,&blockDims.z);
        if (rc != 3){
          throw std::runtime_error("could not parse OSPRAY_DATA_PARALLEL env-var."
              "Must be of format <X>x<Y>x<>Z (e.g., '4x4x4'");
        }
        volume->handle = ospNewVolume("data_distributed_volume");
        if (volume->handle == NULL){
          throw std::runtime_error("#loaders.ospObjectFile: could not create volume ...");
        }
        ospSetVec3i(volume->handle,"num_dp_blocks",blockDims);
      } else {
        // Create the OSPRay object.
        std::cout << "#osp.loader: no OSPRAY_DATA_PARALLEL dimensions set, "
          << "#osp.loader: assuming data replicated mode is desired" << std::endl;
        std::cout << "#osp.loader: to use data parallel mode, set OSPRAY_DATA_PARALLEL env-var to <X>x<Y>x<Z>" << std::endl;
        std::cout << "#osp.loader: where X, Y, and Z are the desired _number_ of data parallel blocks" << std::endl;
        volume->handle = ospNewVolume("block_bricked_volume");
      }
      if (volume->handle == NULL){
        throw std::runtime_error("#loaders.ospObjectFile: could not create volume ...");
      }

      importVolumeRM(fileName, volume);

      // Add the volume to the group
      group->volume.push_back(volume);
    }
  }
}

