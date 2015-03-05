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

#include "MPILoadBalancer.h"
#include "ospray/render/Renderer.h"
#include "ospray/fb/FrameBuffer.h"

namespace ospray {
  namespace mpi {

    using std::cout; 
    using std::endl;

    namespace staticLoadBalancer {

      Master::Master() {
      }

      void Master::renderFrame(Renderer *tiledRenderer,
                               FrameBuffer *fb,
                               const uint32 channelFlags)
      {
        int rc; 
        MPI_Status status;

        // mpidevice already sent the 'cmd_render_frame' event; we
        // only have to wait for tiles...
        const size_t numTiles
          = divRoundUp(fb->size.x,TILE_SIZE)
          * divRoundUp(fb->size.y,TILE_SIZE);
        
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

          ospray::LocalFrameBuffer *lfb = (ospray::LocalFrameBuffer *)fb;
          for (int iy=region.lower.y;iy<region.upper.y;iy++)
            for (int ix=region.lower.x;ix<region.upper.x;ix++) {
              ((uint32*)lfb->colorBuffer)[ix+iy*lfb->size.x] 
                = rgba_i8[iy-region.lower.y][ix-region.lower.x];
            }
        }
        //        printf("#m: master done fb %lx\n",fb);
      }

      void Slave::RenderTask::finish(size_t threadIndex, 
                                     size_t threadCount, 
                                     TaskScheduler::Event* event) 
      {
        renderer->endFrame(channelFlags);
        renderer = NULL;
        fb = NULL;
        // refDec();
      }


      Slave::Slave() {
      }


      void Slave::RenderTask::run(size_t threadIndex, 
                                  size_t threadCount, 
                                  size_t taskIndex, 
                                  size_t taskCount, 
                                  TaskScheduler::Event* event) 
      {
        // PING;
        const size_t tileID = taskIndex;
        if ((tileID % worker.size) != worker.rank) return;

        // PING;
        Tile __aligned(64) tile;
        const size_t tile_y = tileID / numTiles_x;
        const size_t tile_x = tileID - tile_y*numTiles_x;
        tile.region.lower.x = tile_x * TILE_SIZE;
        tile.region.lower.y = tile_y * TILE_SIZE;
        tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fb->size.x);
        tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fb->size.y);
        tile.fbSize = fb->size;
        tile.rcp_fbSize = rcp(vec2f(fb->size));
        renderer->renderTile(tile);
        ospray::LocalFrameBuffer *localFB = (ospray::LocalFrameBuffer *)fb.ptr;
        uint32 rgba_i8[TILE_SIZE][TILE_SIZE];
        for (int iy=tile.region.lower.y;iy<tile.region.upper.y;iy++)
          for (int ix=tile.region.lower.x;ix<tile.region.upper.x;ix++) {
            rgba_i8[iy-tile.region.lower.y][ix-tile.region.lower.x] 
              = ((uint32*)localFB->colorBuffer)[ix+iy*localFB->size.x];
          }
        
        MPI_Send(&tile.region,4,MPI_INT,0,tileID,app.comm);
        int count = (TILE_SIZE)*(TILE_SIZE);
        if (count != 256) PING;
        MPI_Send(&rgba_i8,count,MPI_INT,0,tileID,app.comm);
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
        tiledRenderer->beginFrame(fb);

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

  } // ::ospray::mpi
} // ::ospray
