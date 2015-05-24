#include "PrimStream.h"
#include "ospray/common/TaskSys.h"
#include "common/sys/sync/atomic.h"

namespace ospray {

  const size_t KILO = 1024;
  const size_t MEGA = 1024*KILO;
  const size_t GIGA = 1024*MEGA;
  const size_t LEAF_THRESHOLD = 100*MEGA;

  struct BlockStream : public embree::RefCount {
    BlockStream()
      : numPrims(0), bounds(embree::empty) 
    {
    }
    virtual ~BlockStream()
    { 
      blockRefs.clear(); 
    }
    void clear() { blockRefs.clear(); }
    void push(PrimStream &ts) 
    {
      if (ts.numPrims == 0) return;

      mutex.lock();

      numPrims += ts.numPrims;

      bounds.extend(ts.bounds);
      for (int i=0;i<ts.blockRefs.size();i++)
        blockRefs.push_back(ts.blockRefs[i]);
      mutex.unlock();
    }

    std::vector<Ref<BlockRef> > blockRefs;
    Mutex  mutex;
    size_t numPrims;
    box3f  bounds;
  };

  struct Input : public embree::RefCount {
    virtual void importInto(Ref<BlockStream> bs, size_t myID) = 0;
  };
  
  struct PRefInput : public Input {
    PRefInput(const std::string &fileName) : fileName(fileName) {}
    virtual void importInto(Ref<BlockStream> bs, size_t myID) 
    {
      FILE *file = fopen(fileName.c_str(),"rb");
      assert(file);
      PrimStream ts;
      Prim prim;
      while(fread(&prim,sizeof(prim),1,file) && !feof(file)) {
        ts.push(prim);
      }
      bs->push(ts);
      fclose(file);
    }

    const std::string fileName;
  };

  // struct DummyInput : public Input {
  //   vec3i coord;
  //   vec3i size;
  //   int geomID;

  //   DummyInput(int geomID, vec3i coord, vec3i size)
  //     : geomID(geomID), coord(coord), size(size)
  //   {
  //   }
  //   virtual void importInto(Ref<BlockStream> bs, size_t myID) 
  //   {
  //     PrimStream ts;
  //     size_t primID = 0;
  //     for (int iz=0;iz<size.z;iz++)
  //       for (int iy=0;iy<size.y;iy++)~
  //         for (int ix=0;ix<size.x;ix++) {
  //           vec3f a(coord.x+(ix+drand48())/size.x,
  //                   coord.y+(iy+drand48())/size.y,
  //                   coord.z+(iz+drand48())/size.z);
  //           vec3f b(coord.x+(ix+drand48())/size.x,
  //                   coord.y+(iy+drand48())/size.y,
  //                   coord.z+(iz+drand48())/size.z);
  //           vec3f c(coord.x+(ix+drand48())/size.x,
  //                   coord.y+(iy+drand48())/size.y,
  //                   coord.z+(iz+drand48())/size.z);
  //           ts.push(Prim(geomID,primID++,a,b,c));
  //         }
  //     ts.flush();
  //     bs->push(ts);
  //   }
  // };

  struct ImportTask : public ospray::Task
  {
    std::vector<Ref<Input> > input;
    Ref<BlockStream> output;
    
    ImportTask()
    {
      output = new BlockStream;
    }

    virtual ~ImportTask()
    { 
      output = NULL;
      input.clear(); 
    }

    virtual void run(size_t jobID)
    { 
      size_t myID = (size_t)pthread_self();
      input[jobID]->importInto(output,myID); 
    }
    
  };

  // struct FixedSizePartitionTask : public Task {
    
  //   FixedSizePartitionTask(BlockStream *input, const vec3i &grid)
  //     : grid(grid), input(input), sceneBounds(input->bounds)
  //   {
  //     blockStream.resize(grid.x*grid.y*grid.z);
  //     for (int i=0;i<grid.x*grid.y*grid.z;i++)
  //       blockStream[i] = new BlockStream;
  //   }

  //   virtual ~FixedSizePartitionTask()
  //   {
  //     for (int i=0;i<grid.x*grid.y*grid.z;i++)
  //       blockStream[i] = NULL;
  //   }

  //   template<int dim>
  //   int projectDim(const vec3f &p) const 
  //   {
  //     if (sceneBounds.lower[dim] >= sceneBounds.upper[dim]) return 0;
  //     int i = int(grid[dim]*(p[dim]-sceneBounds.lower[dim])/(sceneBounds.upper[dim]-sceneBounds.lower[dim]+1e-6f));
  //     return std::max(0,std::min(grid[dim]-1,i));
  //   }
    
  //   vec3i project(const vec3f p) const 
  //   {
  //     return vec3i(projectDim<0>(p),projectDim<1>(p),projectDim<2>(p));
  //   }
    
  //   void getIntBounds(vec3i &lo, vec3i &hi, const Prim &prim)
  //   {
  //     box3f primBounds = prim.getBounds();
  //     lo = project(primBounds.lower);
  //     hi = project(primBounds.upper);
  //   }

  //   void pushInputBlock(size_t inputID, std::vector<PrimStream> &cellStream)
  //   {
  //     Block buffer;
  //     Ref<BlockRef> blockRef = input->blockRefs[inputID];
  //     blockRef->loadBlock(buffer);
  //     for (int i=0;i<buffer.prim.size();i++) {
  //       vec3i lo, hi;
  //       const Prim prim = buffer.prim[i];
  //       getIntBounds(lo,hi,prim);
        
  //       for (int iz=lo.z; iz<=hi.z; iz++)
  //         for (int iy=lo.y; iy<=hi.y; iy++)
  //           for (int ix=lo.x; ix<=hi.x; ix++) {
  //             size_t cellID = ix+grid.x*(iy+grid.y*iz);
  //             cellStream[cellID].push(prim);
  //           }
  //     }
  //   }

  //   virtual void run(size_t jobID)
  //   {
  //     std::vector<PrimStream> cellStream;
  //     cellStream.resize(grid.x*grid.y*grid.z);

  //     pushInputBlock(jobID,cellStream);

  //     for (int i=0;i<cellStream.size();i++) {
  //       if (cellStream[i].numPrims > 0) {
  //         cellStream[i].flush();
  //         blockStream[i]->push(cellStream[i]);
  //       }
  //     }
  //   }
    
  //   Ref<BlockStream> input;
  //   std::vector<Ref<BlockStream> > blockStream;
  //   vec3i grid;
  //   box3f sceneBounds;
  // };


  struct BinarySplitTask : public Task {
    
    BinarySplitTask(Ref<BlockStream> input, const box3f &inDomain, const size_t splitID)
      : input(input), splitID(splitID)
    {
      box3f domain = embree::intersect(inDomain,input->bounds);
      lDomain = rDomain = domain;
      int dim = embree::maxDim(domain.size());

      lDomain.upper[dim] = rDomain.lower[dim] = .5f*(domain.lower[dim]+domain.upper[dim]);
      lStream = new BlockStream;
      rStream = new BlockStream;
      
      if (input->numPrims > 10*MEGA) {
        cout << "starting to split " << splitID << ", of " << prettyNumber(input->numPrims) << " bb " << input->bounds << endl << std::flush;
      }
      
    }

    void pushInputBlock(size_t inputID, PrimStream &lPrims, PrimStream &rPrims)
    {
      Block buffer;
      Ref<BlockRef> blockRef = input->blockRefs[inputID];
      blockRef->loadBlock(buffer);

      // PING;
      // PRINT(buffer.prim.size());

      for (int i=0;i<buffer.prim.size();i++) {
        const Prim prim = buffer.prim[i];
        box3f primBounds = prim.getBounds();

        // PRINT(primBounds);
        // PRINT(lDomain);
        // PRINT(rDomain);

        if (!embree::disjoint(lDomain,primBounds))
          lPrims.push(prim);
        if (!embree::disjoint(rDomain,primBounds))
          rPrims.push(prim);

      }
    }

    virtual void run(size_t jobID)
    {
      PrimStream lPrims, rPrims;

      // PING;
      // PRINT(jobID);
      pushInputBlock(jobID,lPrims,rPrims);
      // PRINT(lPrims.numPrims);
      // PRINT(rPrims.numPrims);

      if (lPrims.numPrims > 0) {
        lPrims.flush();
        lStream->push(lPrims);
      }
      if (rPrims.numPrims > 0) {
        rPrims.flush();
        rStream->push(rPrims);
      }
    }
    
    void makeLeaf(Ref<BlockStream> input, const box3f &domain)
    {
      //      static embree::AtomicCounter nextID;
      size_t thisID = splitID; //nextID++;
      char fileName[10000];
      sprintf(fileName,"%s_%015li.dp_leaf",BlockRef::fileNameBase.c_str(),thisID);
      FILE *file = fopen(fileName,"wb");
      fwrite(&input->bounds,sizeof(input->bounds),1,file);
      fwrite(&input->numPrims,sizeof(input->numPrims),1,file);

      for (int blockID=0;blockID<input->blockRefs.size();blockID++) {
        Block buffer;
      
        Ref<BlockRef> blockRef = input->blockRefs[blockID];
        blockRef->loadBlock(buffer);
        for (int i=0;i<buffer.prim.size();i++) {
          const Prim prim = buffer.prim[i];
          fwrite(&prim.primID,sizeof(prim.primID),1,file);
        }
      }
      fclose(file);
      printf("Written leaf block %li, num prims = %li\n",thisID,input->numPrims);
    }

    virtual void finish() 
    {
      if (input->numPrims > 1*MEGA)
        cout << "on split " << splitID << " done splitting " << prettyNumber(input->numPrims)
             << " into " << prettyNumber(lStream->numPrims) << " left and " 
             << prettyNumber(rStream->numPrims) << " right" << endl << std::flush;
      input->clear();
      input = NULL;
      Ref<Task> lTask = finishStream(lStream, lDomain,0);
      Ref<Task> rTask = finishStream(rStream, rDomain,1);
      if (lTask) lTask->wait();
      if (rTask) rTask->wait();

    }
    
    Ref<Task> finishStream(Ref<BlockStream> stream, const box3f &domain, int side)
    {
      if (stream->numPrims < LEAF_THRESHOLD) {
        makeLeaf(stream,domain);
        return NULL;
      } else {
        Ref<Task> ret = new BinarySplitTask(stream,domain,2*splitID+side);
        ret->schedule(stream->blockRefs.size());
        return ret;
      }
    }
    
    Ref<BlockStream> input;
    Ref<BlockStream> lStream;
    Ref<BlockStream> rStream;
    box3f lDomain, rDomain;
    size_t splitID;
  };


  void dpPartition(int ac, char **av)
  {
    Task::initTaskSystem(-1);

    int num_big = 8;
    int num_small = 8;

    Ref<ImportTask> importTask = new ImportTask;
    
    importTask->input.push_back(new PRefInput(av[1]));
    BlockRef::fileNameBase = av[2];

    // int geomID=0;
    // for (int iz=0;iz<num_big;iz++)
    //   for (int iy=0;iy<num_big;iy++)
    //     for (int ix=0;ix<num_big;ix++) {
    //       importTask->input.push_back(new DummyInput(geomID++,vec3i(ix,iy,iz),vec3i(num_small)));
    //     }

    cout << "scheduling import task with " << importTask->input.size() << " inputs" << endl;
    importTask->scheduleAndWait(importTask->input.size());
    cout << "imported " << ospray::prettyNumber(importTask->output->numPrims) << " prims, in " << importTask->output->blockRefs.size() << " blocks" << endl;


    Ref<BinarySplitTask> partTask = new BinarySplitTask(importTask->output,importTask->output->bounds,1);
    importTask = NULL;
    // Ref<FixedSizePartitionTask> partTask = new FixedSizePartitionTask(&importTask->output,vec3i(13,11,7));
    partTask->scheduleAndWait(partTask->input->blockRefs.size());

    // size_t numBricks = 0;
    // for (int i=0;i<partTask->blockStream.size();i++)
    //   numBricks += partTask->blockStream[i]->blockRefs.size();

    // cout << "done fixed-size partitioning, resulted in " << partTask->blockStream.size() << " block streams of " << numBricks << " bricks total" << endl;

    // char line[1000];
    // gets(line);
  }
}

int main(int ac, char **av)
{ ospray::dpPartition(ac,av); }

