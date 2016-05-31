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

// own
#include "Importer.h"
// ospcommon
#include "common/FileName.h"
// ospray api
#include "ospray/ospray.h"
#include "common/sysinfo.h"

// std::
#include <iostream>
#include <stdio.h>
#include <string.h>
#ifdef __LINUX__
# include <sched.h>
#endif
#include <stdint.h>
#include <atomic>
#include <mutex>

namespace ospray {
  namespace vv_importer {
    const static osp::vec3i rmBlockDims{256, 256, 128};

    struct RMLoaderThreads {
      Volume *volume;

      std::mutex mutex;
      std::atomic<int> nextBlockID;
      std::atomic<int> nextPinID;
      int numThreads;
      int timeStep;
      pthread_t *thread;
      std::string inFilesDir;
      bool useGZip;
      // Will: Scaling stuff for DFB benchmarking so we can
      // scale the RM dataset up arbitrarily
      osp::vec3f scaleFactor;
      osp::vec3i scaledBlockDims;

      struct Block {
        uint8_t voxel[256*256*128];
      };

      RMLoaderThreads(Volume *volume, const std::string &fileName, int numThreads, osp::vec3f scaleFactor)
        : volume(volume), nextBlockID(0), thread(NULL), nextPinID(0),
        numThreads(numThreads), scaleFactor(scaleFactor)
      {
        scaledBlockDims.x = scaleFactor.x * rmBlockDims.x;
        scaledBlockDims.y = scaleFactor.y * rmBlockDims.y;
        scaledBlockDims.z = scaleFactor.z * rmBlockDims.z;

        inFilesDir = fileName.substr(0, fileName.rfind('.'));
        std::cout << "Reading LLNL Richtmyer-Meshkov bob from " << inFilesDir
          << " with " << numThreads << " threads\n"
          << "\tBlocks will be scaled from {" << rmBlockDims.x << ", " << rmBlockDims.y
          << ", " << rmBlockDims.z << "} to {" << scaledBlockDims.x << ", "
          << scaledBlockDims.y << ", " << scaledBlockDims.z << "}" << std::endl;

        useGZip = (getenv("OSPRAY_RM_NO_GZIP") == NULL);

        const char *slash = rindex(fileName.c_str(),'/');
        std::string base 
          = slash != NULL
          ? std::string(slash+1)
          : fileName;

        int rc = sscanf(base.c_str(),"bob%03d.bob",&timeStep);
        if (rc != 1)
          throw std::runtime_error("could not extract time step from bob file name "+base);

        volume->voxelRange.x = +std::numeric_limits<float>::infinity();
        volume->voxelRange.y = -std::numeric_limits<float>::infinity();

        thread = new pthread_t[numThreads];
        for (int i=0;i<numThreads;i++)
          pthread_create(thread+i,NULL,(void*(*)(void*))threadFunc,this);

        void *result = NULL;
        for (int i=0;i<numThreads;i++){
          pthread_join(thread[i],&result);
        }
      };

      void loadBlock(Block &block, const std::string &fileNameBase, size_t blockID){
        char fileName[10000];
        FILE *file;
        if (useGZip) {
          sprintf(fileName,"%s/d_%04d_%04li.gz",
              fileNameBase.c_str(),timeStep,blockID);
          const std::string cmd = "gunzip -c "+std::string(fileName);
          file = popen(cmd.c_str(),"r");
          if (!file)
            throw std::runtime_error("could not open file in popen command '"+cmd+"'");
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

        if (useGZip) 
          pclose(file);
        else
          fclose(file);
      }

      // Upsample the 256x256 slice of RM data to scaledBlockDims.x * scaledBlockDims.y,
      // it's assumed that out points to a large enough array to hold the upsampled data
      void upsampleSlice(const uint8_t *slice, uint8_t *out){
        for (int y = 0; y < scaledBlockDims.y; ++y){
          for (int x = 0; x < scaledBlockDims.x; ++x){
            const int sliceIdx = static_cast<int>(y / scaleFactor.y) * rmBlockDims.x + static_cast<int>(x / scaleFactor.x);
            out[y * scaledBlockDims.x + x] = slice[sliceIdx];
          }
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

          int cpu = -1;
#ifdef __LINUX__
          cpu = sched_getcpu();
#endif
          printf("[b%i:%i,%i,%i,(%i)]",blockID,I,J,K,cpu);
          loadBlock(*block,inFilesDir,blockID);

          ospcommon::vec2f blockRange(block->voxel[0]);
          extendVoxelRange(blockRange,&block->voxel[0],256*256*128);
          // Apply scaling to each slice of the data
          uint8_t *scaledSlice = new uint8_t[scaledBlockDims.x * scaledBlockDims.y];
          ospcommon::vec3i region_sz(scaledBlockDims.x, scaledBlockDims.y, 1);
          for (int z = 0; z < scaledBlockDims.z; ++z){
            const int zOrig = static_cast<int>(z / scaleFactor.z);
            ospcommon::vec3i region_lo(I * scaledBlockDims.x, J * scaledBlockDims.y, K * scaledBlockDims.z + z);
            upsampleSlice(&block->voxel[zOrig * rmBlockDims.x * rmBlockDims.y], scaledSlice);
            {
              std::lock_guard<std::mutex> lock(mutex);
              ospSetRegion(volume->handle, scaledSlice, (osp::vec3i&)region_lo, (osp::vec3i&)region_sz);
            }
          }
          {
            std::lock_guard<std::mutex> lock(mutex);
            volume->voxelRange.x = std::min(volume->voxelRange.x, blockRange.x);
            volume->voxelRange.y = std::max(volume->voxelRange.y, blockRange.y);
          }
          delete[] scaledSlice;
        }
        delete block;
      }

      static void *threadFunc(void *arg)
      { ((RMLoaderThreads *)arg)->run(); return NULL; }
      ~RMLoaderThreads() { delete[] thread; };
    };

    // Just import the RM file into some larger scene, just loads the volume
    void importVolumeRM(const FileName &fileName, Volume *volume) {
      const char *scaleFactorEnv = getenv("OSPRAY_RM_SCALE_FACTOR");
      osp::vec3f scaleFactor{1.f, 1.f, 1.f};
      if (scaleFactorEnv){
        std::cout << "#osp.loader: found OSPRAY_RM_SCALE_FACTOR env-var" << std::endl;
        if (sscanf(scaleFactorEnv, "%fx%fx%f", &scaleFactor.x, &scaleFactor.y, &scaleFactor.z) != 3){
          throw std::runtime_error("Could not parse OSPRAY_RM_SCALE_FACTOR env-var. Must be of format"
              "<X>x<Y>x<Z> (e.g '1.5x2x0.5')");
        }
        std::cout << "#osp.loader: got OSPRAY_RM_SCALE_FACTOR env-var = {"
          << scaleFactor.x << ", " << scaleFactor.y << ", " << scaleFactor.z
          << "}" << std::endl;
      }
      // Update the provided dimensions of the volume for the subvolume specified.
      ospcommon::vec3i dims(2048 * scaleFactor.x, 2048 * scaleFactor.y, 1920 * scaleFactor.z);
      volume->dimensions = dims;
      ospSetVec3i(volume->handle, "dimensions", (osp::vec3i&)dims);
      ospSetString(volume->handle,"voxelType", "uchar");

      int numThreads = ospcommon::getNumberOfLogicalThreads();
      const char *numThreadsEnv = getenv("OSPRAY_RM_LOADER_THREADS");
      if (numThreadsEnv){
        numThreads = atoi(numThreadsEnv);
        std::cout << "#osp.loader: Got OSPRAY_RM_LOADER_THREADS env-var, using " << numThreads << "\n";
      } else {
        std::cout << "#osp.loader: No OSPRAY_RM_LOADER_THREADS env-var found, using 8\n";
      }

      double t0 = ospcommon::getSysTime();

      RMLoaderThreads(volume, fileName, numThreads, scaleFactor);
      double t1 = ospcommon::getSysTime();
      std::cout << "done loading " << fileName 
        << ", needed " << (t1-t0) << " seconds" << std::endl;

      ospSet2f(volume->handle,"voxelRange",volume->voxelRange.x,volume->voxelRange.y);
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
          throw std::runtime_error("could not parse OSPRAY_DATA_PARALLEL env-var. Must be of format <X>x<Y>x<>Z (e.g., '4x4x4'");
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

