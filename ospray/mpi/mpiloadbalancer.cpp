#include "mpiloadbalancer.h"
#include "../render/renderer.h"

namespace ospray {
  namespace mpi {
    namespace staticLoadBalancer {
      Master::Master() {
      }
      void Master::renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb)
      {
        // printf("#m: master renders fb %lx\n",fb);
        int rc; MPI_Status status;

        // mpidevice already sent the 'cmd_render_frame' event; we
        // only have to wait for tiles...

        const size_t numTiles
          = divRoundUp(fb->size.x,TILE_SIZE)
          * divRoundUp(fb->size.y,TILE_SIZE);
        
        // printf("MASTER: num tiles %li\n",numTiles);

        __align(64) Tile tile;
        tile.format = TILE_FORMAT_RGBA8;
        tile.fbSize = fb->size;
        for (int i=0;i<numTiles;i++) {
          // printf("#m: receiving tile %i\n",i);
          rc = MPI_Recv(&tile.region,4,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,
                        mpi::worker.comm,&status); 
          Assert(rc == MPI_SUCCESS); 
          // printf("#m: received tile %i (%i,%i) from %i\n",i,
          //        tile.region.lower.x,tile.region.lower.y,status.MPI_SOURCE);
          rc = MPI_Recv(&tile.rgba8[0],TILE_SIZE*TILE_SIZE,MPI_INT,
                        status.MPI_SOURCE,status.MPI_TAG,mpi::worker.comm,&status);
          Assert(rc == MPI_SUCCESS);
          // if (taskIndex == 123*27)
          // printf("#m: %i: %lx\n",(int)status.MPI_SOURCE,tile.rgba8[0]);

           // printf("#m: received tile %i (%i,%i)-(%i,%i) from %i\n",i,
           //        tile.region.lower.x,tile.region.lower.y,
           //        tile.region.upper.x,tile.region.upper.y,
           //        status.MPI_SOURCE);
          fb->setTile(tile);
        }
        // printf("#m: master done fb %lx\n",fb);
      }

      // void Master::returnTile(FrameBuffer *fb, Tile &tile)
      // {
      // }
      
      Slave::RenderTask::RenderTask(size_t numTiles, 
                                    TileRenderer *tiledRenderer,
                                    FrameBuffer *fb)
        : task(&done,_run,this,numTiles,NULL,NULL,//_finish,this,
               "LocalTiledLoadBalancer::RenderTask"),
          fb(fb), 
          tiledRenderer(tiledRenderer)
      {
      }

      Slave::RenderTask::~RenderTask()
      {
      }

      Slave::Slave() {
      }


      void Slave::RenderTask::run(size_t threadIndex, 
                                  size_t threadCount, 
                                  size_t taskIndex, 
                                  size_t taskCount, 
                                  TaskScheduler::Event* event) 
      {
        
        Tile tile;
        tile.fbSize = fb->size;
        const size_t numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
        const size_t tileID = taskIndex * worker.size + worker.rank;
        const size_t tile_y = tileID / numTiles_x;
        const size_t tile_x = tileID % numTiles_x;

        // printf("tileID %i: %i -> %i, %i\n",taskIndex,tileID,tile_x,tile_y);

        tile.region.lower.x = tile_x * TILE_SIZE; //x0;
        tile.region.lower.y = tile_y * TILE_SIZE; //y0;
        // printf("tile %i %i\n",tile_x,tile_y);
        tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fb->size.x);
        tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fb->size.y);

        // printf("tile %i @ %i  : %i %i - %i %i\n",tileID,worker.rank,
        //        tile.region.lower.x,tile.region.lower.y,
        //        tile.region.upper.x,tile.region.upper.y);

        // printf("tile %i %i - %i %i\n",
        //        tile.region.lower.x,tile.region.lower.y,
        //        tile.region.upper.x,tile.region.upper.y);
        tiledRenderer->renderTile(tile);
        
        // for (int i=0;i<TILE_SIZE*TILE_SIZE;i++)
        //   tile.rgba8[i] = 0xffffffff;
          // tile.rgba8[i] = 0x778899aa;
        
        Assert(TiledLoadBalancer::instance);
        Assert(tile.format & TILE_FORMAT_RGBA8);
        // if (taskIndex == 123*27)
        //   printf("%i: %lx\n",worker.rank,tile.rgba8[0]);
#if 1
        MPI_Send(&tile.region,4,MPI_INT,0,tileID,app.comm);
        MPI_Send(&tile.rgba8,TILE_SIZE*TILE_SIZE,MPI_INT,0,tileID,app.comm);
#else
        TiledLoadBalancer::instance->returnTile(fb.ptr,tile);
#endif
        // fb->setTile(tile);
      }
      
      void Slave::renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb)
      {
        const size_t numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
        const size_t numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);
        const size_t numTilesTotal= numTiles_x * numTiles_y;
        const size_t numTilesThisSlave = 1+(numTilesTotal-worker.rank-1) / worker.size;

        // printf("num tiles on slave %li : %li\n",worker.rank,numTilesThisSlave);

        RenderTask renderTask(numTilesThisSlave,tiledRenderer,fb);
        TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, &renderTask.task); 
        // this is actually *waiting* for the task to finish... might
        // not want to do that but somehow try to help render this frame
        renderTask.done.sync();
      }
      // void Slave::returnTile(FrameBuffer *fb, Tile &tile)
      // {
      //   // Assert(tile.format & TILE_FORMAT_RGBA8);
      //   // MPI_Send(&tile.region,4,MPI_INT,0,0,app.comm);
      //   // MPI_Send(&tile.rgba8,4,MPI_INT,0,0,app.comm);
      // }

    }

    DynamicLoadBalancer_Master::DynamicLoadBalancer_Master()
    {
      int rc; MPI_Status status;
      
      // first of all,  the master how many threads we have so he
      // can preallocate some tiles to use...
      slaveInfo = new SlaveInfo[worker.size];
      numSlaveThreads = 0;
      for (int i=0;i<worker.size;i++) {
        rc = MPI_Recv(&slaveInfo[i].numThreads,1,MPI_INT,i,0,worker.comm,&status);
        Assert(rc == MPI_SUCCESS);
        numSlaveThreads += slaveInfo[i].numThreads;
        printf("#m: found slave #%i/%i, with %i threads\n",
               i,worker.size,slaveInfo[i].numThreads);
      }
      printf("#m: found a total of %i slaves, with %i threads total\n",
             worker.size,numSlaveThreads);

      for (int i=0;i<worker.size;i++) {
        rc = MPI_Send(&slaveInfo[i].numThreads,1,MPI_INT,i,0,worker.comm);
        Assert(rc == MPI_SUCCESS);
      }
      PING;
    }

    DynamicLoadBalancer_Slave::DynamicLoadBalancer_Slave()
    {
      int rc; MPI_Status status;

      // first of all, tell the master how many threads we have so he
      // can preallocate some tiles to use...
      int numThreads = TaskScheduler::getNumThreads();
      // printf("#s: slave %i pre-registers at master with %i/%i threads\n",worker.rank,numThreads);
      rc = MPI_Send(&numThreads,1,MPI_INT,0,0,app.comm);
      Assert(rc == MPI_SUCCESS);
      rc = MPI_Recv(&numTotalThreads,1,MPI_INT,0,0,app.comm,&status);
      Assert(rc == MPI_SUCCESS);
      printf("#s: slave %i registered at master with %i/%i threads\n",
             worker.rank,numThreads,numTotalThreads);
      // printf("slave %i got %i tiles preallocated\n",worker.rank,numPreAllocated);
    }

    DynamicLoadBalancer_Slave::SlaveTask::SlaveTask(size_t numTiles, 
                                                    TileRenderer *tiledRenderer,
                                                    FrameBuffer *fb)
      : Task(&doneRendering,_run,this,TaskScheduler::getNumThreads(),NULL,NULL,
             "DynamicLoadBalancer_Slave::SlaveTask"),
        fb(fb), tiledRenderer(tiledRenderer)
    {
    }

    // DynamicLoadBalancer_Master::MasterTask::MasterTask(size_t numTiles, 
    //                                                 TileRenderer *tiledRenderer,
    //                                                 FrameBuffer *fb)
    //   : Task(&fb->doneRendering,_run,this,TaskScheduler::getNumThreads(),NULL,NULL,
    //          "DynamicLoadBalancer_Master::MasterTask"),
    //     fb(fb), tiledRenderer(tiledRenderer)
    // {
    // }

    void DynamicLoadBalancer_Slave::SlaveTask::run(size_t threadIndex, 
                                                   size_t threadCount, 
                                                   size_t taskIndex, 
                                                   size_t taskCount, 
                                                   TaskScheduler::Event* event) 
    {
      int rc; MPI_Status status;

      Tile tile;
      tile.fbSize = fb->size;
      tile.rcp_fbSize = rcp(vec2f(fb->size));
      while (1) {
        int tileID;
        rc = MPI_Recv(&tileID,1,MPI_INT,0,0,app.comm,&status);
        Assert(rc == MPI_SUCCESS);
        if (tileID == -1) 
          // done
          break;
        const size_t numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
        const size_t tile_y = tileID / numTiles_x;
        const size_t tile_x = tileID % numTiles_x;
        tile.region.lower.x = tile_x * TILE_SIZE; //x0;
        tile.region.lower.y = tile_y * TILE_SIZE; //y0;
        tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fb->size.x);
        tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fb->size.y);
        tiledRenderer->renderTile(tile);

        fb->setTile(tile);

        rc = MPI_Send(&tileID,1,MPI_INT,0,0,app.comm);
      }
    }

    void DynamicLoadBalancer_Slave::renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb)
    {
      Assert(tiledRenderer);
      Assert(fb);
      size_t numTiles = divRoundUp(fb->size.x,TILE_SIZE)*divRoundUp(fb->size.y,TILE_SIZE);
      SlaveTask *renderTask = new SlaveTask(numTiles,tiledRenderer,fb);
      TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, renderTask); 
      renderTask->doneRendering.sync();
      PING;
      PING;
      PING;
      delete renderTask; // does not yet kill itself
    }

    // void DynamicLoadBalancer_Slave::returnTile(FrameBuffer *fb, Tile &tile)
    // {
    //   PING;
    // }

    void DynamicLoadBalancer_Master::renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb)
    {
      size_t numTiles = divRoundUp(fb->size.x,TILE_SIZE)*divRoundUp(fb->size.y,TILE_SIZE);
      printf("#m: num tiles %li, num slave threads %i\n",numTiles,numSlaveThreads);
    }

    // void DynamicLoadBalancer_Master::returnTile(FrameBuffer *fb, Tile &tile)
    // {
    //   Assert2(0,"the master should never generate any tiles !?");
    // }

  }
}
