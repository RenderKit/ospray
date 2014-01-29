#include "mpicommon.h"
#include "mpidevice.h"
#include "command.h"
#include "../fb/swapchain.h"
#include "../common/model.h"
#include "../common/data.h"
#include "../geometry/trianglemesh.h"
#include "../render/renderer.h"
#include "../camera/camera.h"
#include "../volume/volume.h"

namespace ospray {
  namespace mpi {
    using std::cout; 
    using std::endl;

    static const int HOST_NAME_MAX = 10000;

    /*! it's up to the proper init
      routine to decide which processes call this function and which
      ones don't. This function will not return. 
      
      \internal We ssume that mpi::worker and mpi::app have already been set up
    */
    void runWorker(int *_ac, const char **_av)
    {
      ospray::init(_ac,&_av);

      MPICommandStream cmd;

      char hostname[HOST_NAME_MAX];
      gethostname(hostname,HOST_NAME_MAX);
      printf("running MPI worker process %i/%i on pid %i@%s\n",
             worker.rank,worker.size,getpid(),hostname);
      int rc;
      while (1) {
        const int command = cmd.get_int32();
        // if (worker.rank == 0)
        //   printf("#w: command %i\n",command);
        usleep(10000);
        switch (command) {
        case api::MPIDevice::CMD_NEW_RENDERER: {
          const ID_t handle = cmd.get_int32();
          const char *type = cmd.get_charPtr();
          if (worker.rank == 0)
            cout << "creating new renderer \"" << type << "\" ID " << handle << endl;
          Renderer *renderer = Renderer::createRenderer(type);
          cmd.free(type);
          Assert(renderer);
          mpi::objectByID[handle] = renderer;
        } break;
        case api::MPIDevice::CMD_FRAMEBUFFER_CREATE: {
          const ID_t   handle = cmd.get_int32();
          const vec2i  size   = cmd.get_vec2i();
          const int32  mode   = cmd.get_int32();
          const size_t swapChainDepth = cmd.get_int32();
          FrameBufferFactory fbFactory = NULL;
          switch(mode) {
          case OSP_RGBA_I8:
            fbFactory = createLocalFB_RGBA_I8;
            break;
          default:
            AssertError("frame buffer mode not yet supported");
          } 
          SwapChain *sc = new SwapChain(swapChainDepth,size,fbFactory);
          mpi::objectByID[handle] = sc;
          Assert(sc != NULL);
        } break;
        case api::MPIDevice::CMD_RENDER_FRAME: {
          const ID_t swapChainID = cmd.get_int32();
          const ID_t rendererID  = cmd.get_int32();
          SwapChain *sc = (SwapChain*)mpi::objectByID[swapChainID].ptr;
          Assert(sc);
          Renderer *renderer = (Renderer*)mpi::objectByID[rendererID].ptr;
          Assert(renderer);
          renderer->renderFrame(sc->getBackBuffer());
          sc->advance();
        } break;
        case api::MPIDevice::CMD_FRAMEBUFFER_MAP: {
          const ID_t scID = cmd.get_int32();
          SwapChain *sc = (SwapChain*)mpi::objectByID[scID].ptr;
          Assert(sc);
          if (worker.rank == 0) {
            // FrameBuffer *fb = sc->getBackBuffer();
            void *ptr = (void*)sc->map();
            rc = MPI_Send(ptr,sc->fbSize.x*sc->fbSize.y,MPI_INT,0,0,mpi::app.comm);
            Assert(rc == MPI_SUCCESS);
            sc->unmap(ptr);
          }
        } break;
        default: 
          std::stringstream err;
          err << "unknown cmd tag " << command;
          throw std::runtime_error(err.str());
        }
      };
    }
  }
}
