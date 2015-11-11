// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include <stdio.h>
#include <string.h>
#include "modules/loaders/RMVolumeFile.h"
#include "ospray/common/OSPCommon.h"
#include "common/sys/sysinfo.h"
#include "common/sys/thread.h"
#ifdef __LINUX__
# include <sched.h>
#endif
#include <stdint.h>

struct RMLoaderThreads {
  OSPVolume volume;
  embree::MutexSys mutex;
  int nextBlockID;
  int numThreads;
  int timeStep;
  pthread_t *thread;
  std::string inFilesDir;

  struct Block {
    uint8_t voxel[256*256*128];
  };


  RMLoaderThreads(OSPVolume volume, const std::string &fileName, int numThreads=10) 
    : volume(volume), nextBlockID(0), thread(NULL),
      numThreads(numThreads)
  {
    PING;
    PRINT(fileName);
    inFilesDir = fileName;

    const char *slash = rindex(fileName.c_str(),'/');
    std::string base 
      = slash != NULL
      ? std::string(slash+1)
      : fileName;
      
    PRINT(base);
    int rc = sscanf(base.c_str(),"bob%03d.bob",&timeStep);
    if (rc != 1)
      throw std::runtime_error("could not extract time step from bob file name "+base);
    PRINT(rc);
    PRINT(timeStep);

    thread = new pthread_t[numThreads];
    for (int i=0;i<numThreads;i++)
      pthread_create(thread+i,NULL,(void*(*)(void*))threadFunc,this);
    void *result = NULL;
    for (int i=0;i<numThreads;i++)
      pthread_join(thread[i],&result);
  };

  void loadBlock(Block &block, const std::string &fileNameBase, size_t blockID)
  {
    char fileName[10000];
    sprintf(fileName,"%s/d_%04d_%04li.gz",fileNameBase.c_str(),timeStep,blockID);

    const std::string cmd = "/usr/bin/gunzip -c "+std::string(fileName);
    // PRINT(cmd);
    FILE *file = popen(cmd.c_str(),"r");
    if (!file)
      throw std::runtime_error("could not open file in popen command '"+cmd+"'");
    assert(file);
    fread(block.voxel,sizeof(uint8_t),256*256*128,file);
    pclose(file);
  }
  
  
  void run() 
  {
    static int nextThreadID = 0;
    mutex.lock();
    int threadID = nextThreadID++;
    embree::setAffinity(threadID);
    mutex.unlock();

    Block *block = new Block;
    while(1) {
      mutex.lock();
      int blockID = nextBlockID++;
      mutex.unlock();
      if (blockID >= 8*8*15) break;

      // int b = K*64+J*8+I;
      int I = blockID % 8;
      int J = (blockID / 8) % 8;
      int K = (blockID / 64);
      
      int cpu = -1;
#ifdef __LINUX__
      cpu = sched_getcpu();
#endif
      printf("[b%i:%i,%i,%i,(%i)]",blockID,I,J,K,cpu); fflush(0);
      int timeStep = 035;
      loadBlock(*block,inFilesDir,blockID);
          
      mutex.lock();
      ospSetRegion(volume,block->voxel,osp::vec3i(I*256,J*256,K*128),osp::vec3i(256,256,128));
      mutex.unlock();
    }
    delete block;
  }

  static void *threadFunc(void *arg)
  { ((RMLoaderThreads *)arg)->run(); return NULL; }
  ~RMLoaderThreads() { delete[] thread; };
};

OSPVolume RMVolumeFile::importVolume(OSPVolume volume)
{
  int numThreads = embree::getNumberOfLogicalThreads(); //20;

  RMLoaderThreads(volume,fileName,numThreads);
  PRINT(fileName);

  // Update the provided dimensions of the volume for the subvolume specified.
  ospSetVec3i(volume, "dimensions", osp::vec3i(2048,2048,1920));
  
  // Return the volume.
  return(volume);
}


