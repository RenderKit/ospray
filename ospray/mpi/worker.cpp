#include "mpicommon.h"
#include "mpidevice.h"
#include "command.h"
//#include "../fb/swapchain.h"
#include "../common/model.h"
#include "../common/data.h"
#include "../common/library.h"
#include "../common/model.h"
#include "../geometry/trianglemesh.h"
#include "../render/renderer.h"
#include "../camera/camera.h"
#include "../volume/volume.h"
#include "mpiloadbalancer.h"

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

      // initialize embree. (we need to do this here rather than in
      // ospray::init() because in mpi-mode the latter is also called
      // in the host-stubs, where it shouldn't.
      std::stringstream embreeConfig;
      if (debugMode)
        embreeConfig << " threads=1";
      rtcInit(embreeConfig.str().c_str());
      assert(rtcGetError() == RTC_NO_ERROR);

      CommandStream cmd;

      char hostname[HOST_NAME_MAX];
      gethostname(hostname,HOST_NAME_MAX);
      printf("#w: running MPI worker process %i/%i on pid %i@%s\n",
             worker.rank,worker.size,getpid(),hostname);
      int rc;


      // TiledLoadBalancer::instance = new mpi::DynamicLoadBalancer_Slave;
      TiledLoadBalancer::instance = new mpi::staticLoadBalancer::Slave;


      while (1) {
        const int command = cmd.get_int32();
#if 0
        if (worker.rank == 0)
          printf("#w: command %i\n",command);
#endif
        switch (command) {
        case api::MPIDevice::CMD_NEW_RENDERER: {
          const mpi::Handle handle = cmd.get_handle();
          const char *type = cmd.get_charPtr();
          if (worker.rank == 0)
            if (logLevel > 2)
              cout << "creating new renderer \"" << type << "\" ID " << handle << endl;
          Renderer *renderer = Renderer::createRenderer(type);
          cmd.free(type);
          Assert(renderer);
          handle.assign(renderer);
        } break;
        case api::MPIDevice::CMD_NEW_CAMERA: {
          const mpi::Handle handle = cmd.get_handle();
          const char *type = cmd.get_charPtr();
          if (worker.rank == 0)
            if (logLevel > 2)
              cout << "creating new camera \"" << type << "\" ID " << (void*)(int64)handle << endl;
          Camera *camera = Camera::createCamera(type);
          cmd.free(type);
          Assert(camera);
          handle.assign(camera);
          //          cout << "#w: new camera " << handle << endl;
        } break;
        case api::MPIDevice::CMD_NEW_VOLUME: {
          // Assert(type != NULL && "invalid volume type identifier");
          const mpi::Handle handle = cmd.get_handle();
          const char *type = cmd.get_charPtr();
          if (worker.rank == 0)
            if (logLevel > 2)
              cout << "creating new volume \"" << type << "\" ID " << (void*)(int64)handle << endl;
          Volume *volume = Volume::createVolume(type);
          if (!volume) 
            throw std::runtime_error("unknown volume type '"+std::string(type)+"'");
          volume->refInc();
          cmd.free(type);
          Assert(volume);
          handle.assign(volume);
          //          cout << "#w: new volume " << handle << endl;
        } break;
        case api::MPIDevice::CMD_NEW_MATERIAL: {
          // Assert(type != NULL && "invalid volume type identifier");
          const mpi::Handle rendererHandle = cmd.get_handle();
          const mpi::Handle handle = cmd.get_handle();
          const char *type = cmd.get_charPtr();
          if (worker.rank == 0)
            if (logLevel > 2)
              cout << "creating new material \"" << type << "\" ID " << (void*)(int64)handle << endl;

          Renderer *renderer = (Renderer *)rendererHandle.lookup();
          Material *material = NULL;
          if (renderer) {
            material = renderer->createMaterial(type);
            if (material) {
              material->refInc();
            }
          }

          if (material == NULL) 
            // no renderer present, or renderer didn't intercept this material.
            material = Material::createMaterial(type);
          
          if (!material) 
            // neither renderer not ospray know this material: throw an error.
            throw std::runtime_error("unknown material type '"+std::string(type)+"'");

          material->refInc();
          cmd.free(type);
          Assert(material);
          handle.assign(material);
          if (worker.rank == 0)
            if (logLevel > 2)
              cout << "#w: new material " << handle << " " << material->toString() << endl;
        } break;

        case api::MPIDevice::CMD_NEW_GEOMETRY: {
          // Assert(type != NULL && "invalid volume type identifier");
          const mpi::Handle handle = cmd.get_handle();
          const char *type = cmd.get_charPtr();
          if (worker.rank == 0)
            if (logLevel > 2)
              cout << "creating new geometry \"" << type << "\" ID " << (void*)(int64)handle << endl;
          Geometry *geometry = Geometry::createGeometry(type);
          if (!geometry) 
            throw std::runtime_error("unknown geometry type '"+std::string(type)+"'");
          geometry->refInc();
          cmd.free(type);
          Assert(geometry);
          handle.assign(geometry);
          if (worker.rank == 0)
            if (logLevel > 2)
	      cout << "#w: new geometry " << handle << " " << geometry->toString() << endl;
        } break;

        case api::MPIDevice::CMD_FRAMEBUFFER_CREATE: {
          const mpi::Handle handle = cmd.get_handle();
          const vec2i  size   = cmd.get_vec2i();
          const int32  mode   = cmd.get_int32();
          //          const size_t swapChainDepth = cmd.get_int32();
          // FrameBufferFactory fbFactory = NULL;
          // switch(mode) {
          // case OSP_RGBA_I8:
          //   fbFactory = createLocalFB_RGBA_I8;
          //   break;
          // default:
          //   AssertError("frame buffer mode not yet supported");
          // } 
          FrameBuffer *fb = new LocalFrameBuffer<uint32>(size);
          // SwapChain *sc = new SwapChain(swapChainDepth,size,fbFactory);
          handle.assign(fb);
          // Assert(sc != NULL);
        } break;
        case api::MPIDevice::CMD_RENDER_FRAME: {
          const mpi::Handle  fbHandle = cmd.get_handle();
          // const mpi::Handle  swapChainHandle = cmd.get_handle();
          const mpi::Handle  rendererHandle  = cmd.get_handle();
          FrameBuffer *fb = (FrameBuffer*)fbHandle.lookup();
          // SwapChain *sc = (SwapChain*)swapChainHandle.lookup();
          // Assert(sc);
          Renderer *renderer = (Renderer*)rendererHandle.lookup();
          Assert(renderer);
          renderer->renderFrame(fb); //sc->getBackBuffer());
          // sc->advance();
        } break;
        case api::MPIDevice::CMD_FRAMEBUFFER_MAP: {
          const mpi::Handle handle = cmd.get_handle();
          FrameBuffer *fb = (FrameBuffer*)handle.lookup();
          // SwapChain *sc = (SwapChain*)handle.lookup();
          // Assert(sc);
          if (worker.rank == 0) {
            // FrameBuffer *fb = sc->getBackBuffer();
            void *ptr = (void*)fb->map();
            // void *ptr = (void*)sc->map();
            rc = MPI_Send(ptr,fb->size.x*fb->size.y,MPI_INT,0,0,mpi::app.comm);
            Assert(rc == MPI_SUCCESS);
            fb->unmap(ptr);
          }
        } break;
        case api::MPIDevice::CMD_NEW_MODEL: {
          const mpi::Handle handle = cmd.get_handle();
          Model *model = new Model;
          Assert(model);
          handle.assign(model);
          if (logLevel > 2)
            cout << "#w: new model " << handle << endl;
        } break;
        case api::MPIDevice::CMD_NEW_TRIANGLEMESH: {
          const mpi::Handle handle = cmd.get_handle();
          TriangleMesh *triangleMesh = new TriangleMesh;
          Assert(triangleMesh);
          handle.assign(triangleMesh);
        } break;
        case api::MPIDevice::CMD_NEW_DATA: {
          const mpi::Handle handle = cmd.get_handle();
          Data *data = NULL;
          size_t nitems      = cmd.get_size_t();
          OSPDataType format = (OSPDataType)cmd.get_int32();
          int flags          = cmd.get_int32();
          data = new Data(nitems,format,NULL,flags & ~OSP_DATA_SHARED_BUFFER);
          Assert(data);
          handle.assign(data);

          size_t hasInitData = cmd.get_size_t();
          if (hasInitData) {
            cmd.get_data(nitems*sizeOf(format),data->data);
            if (format==OSP_OBJECT) {
              /* translating handles to managedobject pointers: if a
                 data array has 'object' or 'data' entry types, then
                 what the host sends are _handles_, not pointers, but
                 what the core expects are pointers; to make the core
                 happy we translate all data items back to pointers at
                 this stage */
              mpi::Handle    *asHandle = (mpi::Handle    *)data->data;
              ManagedObject **asObjPtr = (ManagedObject **)data->data;
              for (int i=0;i<nitems;i++) {
                if (asHandle[i] != NULL_HANDLE) {
                  asObjPtr[i] = asHandle[i].lookup();
                  asObjPtr[i]->refInc();
                }
              }
            }
          }
        } break;
        case api::MPIDevice::CMD_ADD_GEOMETRY: {
          const mpi::Handle modelHandle = cmd.get_handle();
          const mpi::Handle geomHandle = cmd.get_handle();
          Model *model = (Model*)modelHandle.lookup();
          Assert(model);
          Geometry *geom = (Geometry*)geomHandle.lookup();
          Assert(geom);
          model->geometry.push_back(geom);
        } break;
        case api::MPIDevice::CMD_COMMIT: {
          const mpi::Handle handle = cmd.get_handle();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          //          printf("#w%i:c%i",worker.rank,(int)handle);
          if (logLevel > 2)
            cout << "#w: committing " << handle << " " << obj->toString() << endl;
          obj->commit();

          // hack, to stay compatible with earlier version
          Model *model = dynamic_cast<Model *>(obj);
          if (model)
            model->finalize();

        } break;
        case api::MPIDevice::CMD_SET_OBJECT: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();
          const mpi::Handle val = cmd.get_handle();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          obj->setParam(name,val.lookup());
          cmd.free(name);
        } break;
        case api::MPIDevice::CMD_RELEASE: {
          const mpi::Handle handle = cmd.get_handle();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          handle.freeObject();
        } break;
        case api::MPIDevice::CMD_SET_MATERIAL: {
          const mpi::Handle geoHandle = cmd.get_handle();
          const mpi::Handle matHandle = cmd.get_handle();
          Geometry *geo = (Geometry*)geoHandle.lookup();
          Material *mat = (Material*)matHandle.lookup();
          geo->setMaterial(mat);
        } break;
        case api::MPIDevice::CMD_SET_STRING: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();
          const char *val  = cmd.get_charPtr();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          obj->findParam(name,1)->set(val);
          cmd.free(name);
          cmd.free(val);
        } break;
        case api::MPIDevice::CMD_SET_INT: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();
          const int val = cmd.get_int();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          obj->findParam(name,1)->set(val);
          cmd.free(name);
        } break;
        case api::MPIDevice::CMD_SET_FLOAT: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();
          const float val = cmd.get_float();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          obj->findParam(name,1)->set(val);
          cmd.free(name);
        } break;
        case api::MPIDevice::CMD_SET_VEC3F: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();
          const vec3f val = cmd.get_vec3f();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          obj->findParam(name,1)->set(val);
          cmd.free(name);
        } break;
        case api::MPIDevice::CMD_SET_VEC3I: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();
          const vec3i val = cmd.get_vec3i();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          obj->findParam(name,1)->set(val);
          cmd.free(name);
        } break;
        case api::MPIDevice::CMD_LOAD_MODULE: {
          const char *name = cmd.get_charPtr();

#if THIS_IS_MIC
          // embree automatically puts this into "lib<name>.so" format
          std::string libName = "ospray_module_"+std::string(name)+"_mic";
#else
          std::string libName = "ospray_module_"+std::string(name)+"";
#endif
          loadLibrary(libName);
      
          std::string initSymName = "ospray_init_module_"+std::string(name);
          void*initSym = getSymbol(initSymName);
          if (!initSym)
            throw std::runtime_error("could not find module initializer "+initSymName);
          void (*initMethod)() = (void(*)())initSym;
          initMethod();

          cmd.free(name);
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
