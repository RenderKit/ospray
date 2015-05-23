#include "TriangleStream.h"
#include "ospray/common/TaskSys.h"

namespace ospray {

  const int LEAF_THRESHOLD = 10000;

  struct BlockStream : public embree::RefCount {
    BlockStream()
      : numTriangles(0), bounds(embree::empty) 
    {}
    
    void push(TriangleStream &ts) 
    {
      if (ts.numTriangles == 0) return;

      mutex.lock();

      numTriangles += ts.numTriangles;
      bounds.extend(ts.bounds);

      for (int i=0;i<ts.blockRefs.size();i++)
        blockRefs.push_back(ts.blockRefs[i]);
      mutex.unlock();
    }

    std::vector<Ref<BlockRef> > blockRefs;
    Mutex  mutex;
    size_t numTriangles;
    box3f  bounds;
  };

  struct Input : public embree::RefCount {
    virtual void importInto(Ref<BlockStream> bs, size_t myID) = 0;
  };

  struct DummyInput : public Input {
    vec3i coord;
    vec3i size;
    int geomID;

    DummyInput(int geomID, vec3i coord, vec3i size)
      : geomID(geomID), coord(coord), size(size)
    {
    }
    virtual void importInto(Ref<BlockStream> bs, size_t myID) 
    {
      TriangleStream ts;
      size_t triangleID = 0;
      for (int iz=0;iz<size.z;iz++)
        for (int iy=0;iy<size.y;iy++)
          for (int ix=0;ix<size.x;ix++) {
            vec3f a(coord.x+(ix+drand48())/size.x,
                    coord.y+(iy+drand48())/size.y,
                    coord.z+(iz+drand48())/size.z);
            vec3f b(coord.x+(ix+drand48())/size.x,
                    coord.y+(iy+drand48())/size.y,
                    coord.z+(iz+drand48())/size.z);
            vec3f c(coord.x+(ix+drand48())/size.x,
                    coord.y+(iy+drand48())/size.y,
                    coord.z+(iz+drand48())/size.z);
            ts.push(Triangle(geomID,triangleID++,a,b,c));
          }
      ts.flush();
      bs->push(ts);
    }
  };

  struct ImportTask : public ospray::Task
  {
    std::vector<Ref<Input> > input;
    Ref<BlockStream> output;
    
    ImportTask()
      : output(new BlockStream)
    {}

    virtual void run(size_t jobID)
    { 
      size_t myID = (size_t)pthread_self();

      input[jobID]->importInto(output,myID); 
    }
    
  };

  struct FixedSizePartitionTask : public Task {
    
    FixedSizePartitionTask(BlockStream *input, const vec3i &grid)
      : grid(grid), input(input), sceneBounds(input->bounds)
    {
      blockStream.resize(grid.x*grid.y*grid.z);
      for (int i=0;i<grid.x*grid.y*grid.z;i++)
        blockStream[i] = new BlockStream;
    }

    virtual ~FixedSizePartitionTask()
    {
      for (int i=0;i<grid.x*grid.y*grid.z;i++)
        blockStream[i] = NULL;
    }

    template<int dim>
    int projectDim(const vec3f &p) const 
    {
      if (sceneBounds.lower[dim] >= sceneBounds.upper[dim]) return 0;
      int i = int(grid[dim]*(p[dim]-sceneBounds.lower[dim])/(sceneBounds.upper[dim]-sceneBounds.lower[dim]+1e-6f));
      return std::max(0,std::min(grid[dim]-1,i));
    }
    
    vec3i project(const vec3f p) const 
    {
      return vec3i(projectDim<0>(p),projectDim<1>(p),projectDim<2>(p));
    }
    
    void getIntBounds(vec3i &lo, vec3i &hi, const Triangle &tri)
    {
      box3f triBounds = embree::empty;
      triBounds.extend(tri.a);
      triBounds.extend(tri.b);
      triBounds.extend(tri.c);
      lo = project(triBounds.lower);
      hi = project(triBounds.upper);
    }

    void pushInputBlock(size_t inputID, std::vector<TriangleStream> &cellStream)
    {
      Block buffer;
      Ref<BlockRef> blockRef = input->blockRefs[inputID];
      blockRef->loadBlock(buffer);
      for (int i=0;i<buffer.triangle.size();i++) {
        vec3i lo, hi;
        const Triangle tri = buffer.triangle[i];
        getIntBounds(lo,hi,tri);
        
        for (int iz=lo.z; iz<=hi.z; iz++)
          for (int iy=lo.y; iy<=hi.y; iy++)
            for (int ix=lo.x; ix<=hi.x; ix++) {
              size_t cellID = ix+grid.x*(iy+grid.y*iz);
              cellStream[cellID].push(tri);
            }
      }
    }

    virtual void run(size_t jobID)
    {
      std::vector<TriangleStream> cellStream;
      cellStream.resize(grid.x*grid.y*grid.z);

      pushInputBlock(jobID,cellStream);

      for (int i=0;i<cellStream.size();i++) {
        if (cellStream[i].numTriangles > 0) {
          cellStream[i].flush();
          blockStream[i]->push(cellStream[i]);
        }
      }
    }
    
    Ref<BlockStream> input;
    std::vector<Ref<BlockStream> > blockStream;
    vec3i grid;
    box3f sceneBounds;
  };


  struct BinarySplitTask : public Task {
    
    BinarySplitTask(Ref<BlockStream> input, const box3f &inDomain)
      : input(input)
    {
      box3f domain = embree::intersect(inDomain,input->bounds);
      lDomain = rDomain = domain;
      int dim = embree::maxDim(domain.size());

      lDomain.upper[dim] = rDomain.lower[dim] = .5f*(domain.lower[dim]+domain.upper[dim]);
      lStream = new BlockStream;
      rStream = new BlockStream;
    }

    virtual ~BinarySplitTask()
    {
      lStream = NULL;
      rStream = NULL;
    }

    void pushInputBlock(size_t inputID, TriangleStream &lTris, TriangleStream &rTris)
    {
      Block buffer;
      Ref<BlockRef> blockRef = input->blockRefs[inputID];
      blockRef->loadBlock(buffer);

      // PING;
      // PRINT(buffer.triangle.size());

      for (int i=0;i<buffer.triangle.size();i++) {
        const Triangle tri = buffer.triangle[i];
        box3f triBounds = tri.getBounds();

        // PRINT(triBounds);
        // PRINT(lDomain);
        // PRINT(rDomain);

        if (!embree::disjoint(lDomain,triBounds))
          lTris.push(tri);
        if (!embree::disjoint(rDomain,triBounds))
          rTris.push(tri);

      }
    }

    virtual void run(size_t jobID)
    {
      TriangleStream lTris, rTris;

      // PING;
      // PRINT(jobID);
      pushInputBlock(jobID,lTris,rTris);
      // PRINT(lTris.numTriangles);
      // PRINT(rTris.numTriangles);

      if (lTris.numTriangles > 0) {
        lTris.flush();
        lStream->push(lTris);
      }
      if (rTris.numTriangles > 0) {
        rTris.flush();
        rStream->push(rTris);
      }
    }
    
    // Ref<Task> finishStream(Ref<BlockStream> stream, const box3f &domain)
    // {
    //   box3f trueBounds = embree::intersect(stream,domain);
      
    // }

    void makeMesh(Ref<BlockStream> &blocks, const box3f &domain)
    {
      cout << "MAKING MESH " << blocks->numTriangles << endl;
    }

    virtual void finish() 
    {
      // PING;
      // PRINT(lStream->numTriangles);
      // PRINT(rStream->numTriangles);
      Ref<Task> lTask = finishStream(lStream, lDomain);
      Ref<Task> rTask = finishStream(rStream, rDomain);
      if (lTask) lTask->wait();
      if (rTask) rTask->wait();
    }
    
    Ref<Task> finishStream(Ref<BlockStream> &stream, const box3f &domain)
    {
      if (stream->numTriangles < LEAF_THRESHOLD) {
        makeMesh(stream,domain);
        return NULL;
      } else {
        Ref<Task> ret = new BinarySplitTask(stream,domain);
        ret->schedule(stream->blockRefs.size());
        return ret;
      }
    }
    
    Ref<BlockStream> input;
    Ref<BlockStream> lStream;
    Ref<BlockStream> rStream;
    box3f lDomain, rDomain;
  };


  void dpPartition(int ac, char **av)
  {
    Task::initTaskSystem(0);

    int num_big = 8;
    int num_small = 8;

    Ref<ImportTask> importTask = new ImportTask;

    int geomID=0;
    for (int iz=0;iz<num_big;iz++)
      for (int iy=0;iy<num_big;iy++)
        for (int ix=0;ix<num_big;ix++) {
          importTask->input.push_back(new DummyInput(geomID++,vec3i(ix,iy,iz),vec3i(num_small)));
        }

    cout << "scheduling import task with " << importTask->input.size() << " inputs" << endl;
    importTask->scheduleAndWait(importTask->input.size());
    cout << "imported " << importTask->output->numTriangles << " triangles, in " << importTask->output->blockRefs.size() << " blocks" << endl;


    Ref<BinarySplitTask> partTask = new BinarySplitTask(importTask->output,importTask->output->bounds);
    // Ref<FixedSizePartitionTask> partTask = new FixedSizePartitionTask(&importTask->output,vec3i(13,11,7));
    partTask->scheduleAndWait(importTask->output->blockRefs.size());

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

