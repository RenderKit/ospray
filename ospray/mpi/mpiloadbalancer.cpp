#include "mpiloadbalancer.h"
#include "ospray/render/renderer.h"
#include "ospray/fb/framebuffer.h"

namespace ospray {
  namespace mpi {
    namespace staticLoadBalancer {
      Master::Master() {
      }
      void Master::renderFrame(Renderer *tiledRenderer,
                               FrameBuffer *fb,
                               const uint32 channelFlags)
      {
        int rc; MPI_Status status;

        // mpidevice already sent the 'cmd_render_frame' event; we
        // only have to wait for tiles...

        const size_t numTiles
          = divRoundUp(fb->size.x,TILE_SIZE)
          * divRoundUp(fb->size.y,TILE_SIZE);
        
        // printf("MASTER: num tiles %li\n",numTiles);

        // tile.format = TILE_FORMAT_RGBA8;
        // tile.fbSize = fb->size;
        // tile.rcp_fbSize = rcp(vec2f(fb->size));
        assert(fb->colorBufferFormat == OSP_RGBA_I8);
        uint32 rgba_i8[TILE_SIZE][TILE_SIZE];
        for (int i=0;i<numTiles;i++) {
          box2ui region;
	  // printf("#m: receiving tile %i\n",i);
          rc = MPI_Recv(&region,4,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,
                        mpi::worker.comm,&status); 
          Assert(rc == MPI_SUCCESS); 
           // printf("#m: received tile %i (%i,%i) from %i\n",i,
           //        tile.region.lower.x,tile.region.lower.y,status.MPI_SOURCE);
          rc = MPI_Recv(&rgba_i8[0],TILE_SIZE*TILE_SIZE,MPI_INT,
                        status.MPI_SOURCE,status.MPI_TAG,mpi::worker.comm,&status);
          Assert(rc == MPI_SUCCESS);
          // if (taskIndex == 123*27)
          // printf("#m: %i: %lx\n",(int)status.MPI_SOURCE,tile.rgba8[0]);

          // printf("#m: received tile %i (%i,%i)-(%i,%i) from %i\n",i,
          //        tile.region.lower.x,tile.region.lower.y,
          //        tile.region.upper.x,tile.region.upper.y,
          //        status.MPI_SOURCE);

          // printf("tile %i/%i\n",i,numTiles);
          // fb->setTile(tile);

          ospray::LocalFrameBuffer *lfb = (ospray::LocalFrameBuffer *)fb;
          for (int iy=region.lower.y;iy<region.upper.y;iy++)
            for (int ix=region.lower.x;ix<region.upper.x;ix++)
              ((uint32*)lfb->colorBuffer)[ix+iy*lfb->size.x] 
                = rgba_i8[iy-region.lower.y][ix-region.lower.x];
        }
        // printf("#m: master done fb %lx\n",fb);
      }

      // void Master::returnTile(FrameBuffer *fb, Tile &tile)
      // {
      // }
      
      // Slave::RenderTask::RenderTask(FrameBuffer *fb,
      //                               TileRenderer::RenderJob *frameRenderJob)
      //   : fb(fb),
      //     numTiles_x(divRoundUp(fb->size.x,TILE_SIZE)),
      //     numTiles_y(divRoundUp(fb->size.y,TILE_SIZE)),
      //     fbSize(fb->size),
      //     task(&done,_run,this,
      //          divRoundUp(fb->size.x,TILE_SIZE)*divRoundUp(fb->size.y,TILE_SIZE),
      //          _finish,this,
      //          "MPILoadBalancer::Slave::RenderTask"),
      //     frameRenderJob(frameRenderJob)
      // {
      //   // refInc();
      // }

      void Slave::RenderTask::finish(size_t threadIndex, 
                                     size_t threadCount, 
                                     TaskScheduler::Event* event) 
      {
        renderer->endFrame(channelFlags);
        renderer = NULL;
        fb = NULL;
        // refDec();
      }

      // RenderTask(size_t numTiles, 
      //                                     TileRenderer *tiledRenderer,
      //                                     FrameBuffer *fb)
      //         : task(&done,_run,this,numTiles,NULL,NULL,//_finish,this,
      //                "LocalTiledLoadBalancer::RenderTask"),
      //           fb(fb), 
      //           tiledRenderer(tiledRenderer)
      //       {
      //       }

      // Slave::RenderTask::~RenderTask()
      // {
      // }

      Slave::Slave() {
      }


      void Slave::RenderTask::run(size_t threadIndex, 
                                  size_t threadCount, 
                                  size_t taskIndex, 
                                  size_t taskCount, 
                                  TaskScheduler::Event* event) 
      {
        const size_t tileID = taskIndex;
        if ((tileID % worker.size) != worker.rank) return;

        Tile tile;
        const size_t tile_y = tileID / numTiles_x;
        const size_t tile_x = tileID - tile_y*numTiles_x;
        region2ui region;
        region.lower.x = tile_x * TILE_SIZE;
        region.lower.y = tile_y * TILE_SIZE;
        region.upper.x = std::min(region.lower.x+TILE_SIZE,(uint32)fb->size.x);
        region.upper.y = std::min(region.lower.y+TILE_SIZE,(uint32)fb->size.y);
        renderer->renderTile(tile);

        ospray::LocalFrameBuffer *localFB = (ospray::LocalFrameBuffer *)fb.ptr;
        uint32 rgba_i8[TILE_SIZE][TILE_SIZE];
        for (int iy=region.lower.y;iy<region.upper.y;iy++)
          for (int ix=region.lower.x;ix<region.upper.x;ix++)
            rgba_i8[iy-region.lower.y][ix-region.lower.x] 
              = ((uint32*)localFB->colorBuffer)[ix+iy*localFB->size.x];
        
        MPI_Send(&region,4,MPI_INT,0,tileID,app.comm);
        MPI_Send(&rgba_i8,TILE_SIZE*TILE_SIZE,MPI_INT,0,tileID,app.comm);
      }
      
      void Slave::renderFrame(Renderer *tiledRenderer, 
                              FrameBuffer *fb,
                              const uint32 channelFlags
                              )
      {
        Ref<RenderTask> renderTask
          = new RenderTask;//(fb,tiledRenderer->createRenderJob(fb));
        renderTask->fb = fb;
        renderTask->renderer = tiledRenderer;
        renderTask->numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
        renderTask->numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);
        renderTask->channelFlags = channelFlags;
        
        /*! iw: using a local sync event for now; "in theory" we should be
          able to attach something like a sync event to the frame
          buffer, just trigger the task here, and let somebody else sync
          on the framebuffer once it is needed; alas, I'm currently
          running into some issues with the embree taks system when
          trying to do so, and thus am reverting to this
          fully-synchronous version for now */
        
        // renderTask->fb->frameIsReadyEvent = TaskScheduler::EventSync();
        TaskScheduler::EventSync sync;
        renderTask->task = embree::TaskScheduler::Task
          (&sync,
           // (&renderTask->fb->frameIsReadyEvent,
           renderTask->_run,renderTask.ptr,
           renderTask->numTiles_x*renderTask->numTiles_y,
           renderTask->_finish,renderTask.ptr,
           "LocalTiledLoadBalancer::RenderTask");
        TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, &renderTask->task); 
        sync.sync();
      }
    }

#if 0
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
        tiledRenderer->frameRenderJob->renderTile(tile);

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

#endif
  }

}
