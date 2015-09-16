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

#include "MPICommon.h"
#include "MPIDevice.h"
#include "CommandStream.h"
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/common/Library.h"
#include "ospray/common/Model.h"
#include "ospray/geometry/TriangleMesh.h"
#include "ospray/render/Renderer.h"
#include "ospray/camera/Camera.h"
#include "ospray/volume/Volume.h"
#include "ospray/lights/Light.h"
#include "ospray/texture/Texture2D.h"
#include "MPILoadBalancer.h"
#include "ospray/transferFunction/TransferFunction.h"
// std
#include <algorithm>
#include <unistd.h> // for gethostname()

namespace ospray {
  namespace mpi {
    using std::cout; 
    using std::endl;

    static const int HOST_NAME_MAX = 10000;

    struct GeometryLocator {
      bool operator()(const embree::Ref<ospray::Geometry> &g) const {
        return ptr == &*g;
      }
      Geometry *ptr;
    };

    void embreeErrorFunc(const RTCError code, const char* str)
    {
      std::cerr << "#osp: embree internal error " << code << " : " << str << std::endl;
      throw std::runtime_error("embree internal error '"+std::string(str)+"'");
    }


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
      // std::stringstream embreeConfig;
      // if (debugMode)
      //   embreeConfig << " threads=1";
      // rtcInit(embreeConfig.str().c_str());

      //      assert(rtcGetError() == RTC_NO_ERROR);
      rtcSetErrorFunction(embreeErrorFunc);

      std::stringstream embreeConfig;
      if (debugMode)
        embreeConfig << " threads=1,verbose=2";
      else if(numThreads > 0)
        embreeConfig << " threads=" << numThreads;
      rtcInit(embreeConfig.str().c_str());

      if (rtcGetError() != RTC_NO_ERROR) {
        // why did the error function not get called !?
        std::cerr << "#osp:init: embree internal error number " << (int)rtcGetError() << std::endl;
        assert(rtcGetError() == RTC_NO_ERROR);
      }


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
          Volume *volume = Volume::createInstance(type);
          if (!volume) 
            throw std::runtime_error("unknown volume type '"+std::string(type)+"'");
          volume->refInc();
          cmd.free(type);
          Assert(volume);
          handle.assign(volume);
          //          cout << "#w: new volume " << handle << endl;
        } break;
        case api::MPIDevice::CMD_NEW_TRANSFERFUNCTION: {
          const mpi::Handle handle = cmd.get_handle();
          const char *type = cmd.get_charPtr();
          if (worker.rank == 0)
            if (logLevel > 2)
              cout << "creating new transfer function \"" << type << "\" ID " << (void*)(int64)handle << endl;
          TransferFunction *transferFunction = TransferFunction::createInstance(type);
          if (!transferFunction) {
            throw std::runtime_error("unknown transfer function type '"+std::string(type)+"'");
          }
          transferFunction->refInc();
          cmd.free(type);
          handle.assign(transferFunction);
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
          
          int myFail = (material == NULL);
          int sumFail = 0;
          rc = MPI_Allreduce(&myFail,&sumFail,1,MPI_INT,MPI_SUM,worker.comm);
          if (sumFail == 0) {
            material->refInc();
            cmd.free(type);
            Assert(material);
            handle.assign(material);
            if (worker.rank == 0) {
              if (logLevel > 2)
                cout << "#w: new material " << handle << " " 
                     << material->toString() << endl;
              MPI_Send(&sumFail,1,MPI_INT,0,0,mpi::app.comm);
            }
          } else { 
            // at least one client could not load/create material ...
            if (material) material->refDec();
            if (worker.rank == 0) {
              if (logLevel > 2)
                cout << "#w: could not create material " << handle << " " 
                     << material->toString() << endl;
              MPI_Send(&sumFail,1,MPI_INT,0,0,mpi::app.comm);
            }
          }
        } break;

        case api::MPIDevice::CMD_NEW_LIGHT: {
          const mpi::Handle rendererHandle = cmd.get_handle();
          const mpi::Handle handle = cmd.get_handle();
          const char *type = cmd.get_charPtr();
          if (worker.rank == 0)
            if (logLevel > 2)
              cout << "creating new light \"" << type << "\" ID " << (void*)(int64)handle << endl;

          Renderer *renderer = (Renderer *)rendererHandle.lookup();
          Light *light = NULL;
          if (renderer) {
            light = renderer->createLight(type);
            if (light) {
              light->refInc();
            }
          }
          if (light == NULL) 
            // no renderer present, or renderer didn't intercept this light.
            light = Light::createLight(type);

          int myFail = (light == NULL);
          int sumFail = 0;
          rc = MPI_Allreduce(&myFail,&sumFail,1,MPI_INT,MPI_SUM,worker.comm);
          if (sumFail == 0) {
            light->refInc();
            cmd.free(type);
            Assert(light);
            handle.assign(light);
            if (worker.rank == 0) {
              if (logLevel > 2)
                cout << "#w: new light " << handle << " " 
                     << light->toString() << endl;
              MPI_Send(&sumFail,1,MPI_INT,0,0,mpi::app.comm);
            }
          } else { 
            // at least one client could not load/create light ...
            if (light) light->refDec();
            if (worker.rank == 0) {
              if (logLevel > 2)
                cout << "#w: could not create light " << handle << " " 
                     << light->toString() << endl;
              MPI_Send(&sumFail,1,MPI_INT,0,0,mpi::app.comm);
            }
          }
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
          const vec2i  size               = cmd.get_vec2i();
          const OSPFrameBufferFormat mode = (OSPFrameBufferFormat)cmd.get_int32();
          const uint32 channelFlags       = cmd.get_int32();
          bool hasDepthBuffer = (channelFlags & OSP_FB_DEPTH);
          bool hasAccumBuffer = (channelFlags & OSP_FB_ACCUM);
          FrameBuffer *fb = new LocalFrameBuffer(size,mode,hasDepthBuffer,hasAccumBuffer);
          handle.assign(fb);
        } break;
        case api::MPIDevice::CMD_FRAMEBUFFER_CLEAR: {
          const mpi::Handle handle = cmd.get_handle();
          const uint32 channelFlags       = cmd.get_int32();
          FrameBuffer *fb = (FrameBuffer*)handle.lookup();
          fb->clear(channelFlags);
        } break;
        case api::MPIDevice::CMD_RENDER_FRAME: {
          const mpi::Handle  fbHandle = cmd.get_handle();
          // const mpi::Handle  swapChainHandle = cmd.get_handle();
          const mpi::Handle  rendererHandle  = cmd.get_handle();
          const uint32 channelFlags          = cmd.get_int32();
          FrameBuffer *fb = (FrameBuffer*)fbHandle.lookup();
          // SwapChain *sc = (SwapChain*)swapChainHandle.lookup();
          // Assert(sc);
          Renderer *renderer = (Renderer*)rendererHandle.lookup();
          Assert(renderer);
          renderer->renderFrame(fb,channelFlags); //sc->getBackBuffer());
          // sc->advance();
        } break;
        case api::MPIDevice::CMD_FRAMEBUFFER_MAP: {
          FATAL("should never get called on worker!?");
          // const mpi::Handle handle = cmd.get_handle();
          // FrameBuffer *fb = (FrameBuffer*)handle.lookup();
          // // SwapChain *sc = (SwapChain*)handle.lookup();
          // // Assert(sc);
          // if (worker.rank == 0) {
          //   // FrameBuffer *fb = sc->getBackBuffer();
          //   void *ptr = (void*)fb->map();
          //   // void *ptr = (void*)sc->map();
          //   rc = MPI_Send(ptr,fb->size.x*fb->size.y,MPI_INT,0,0,mpi::app.comm);
          //   Assert(rc == MPI_SUCCESS);
          //   fb->unmap(ptr);
          // }
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

        case api::MPIDevice::CMD_NEW_TEXTURE2D: {
          const mpi::Handle handle = cmd.get_handle();
          Texture2D *texture2D = NULL;

          int32 width = cmd.get_int32();
          int32 height = cmd.get_int32();
          int32 type = cmd.get_int32();
          int32 flags = cmd.get_int32();
          size_t size = cmd.get_size_t();
          
          void *data = malloc(size);
          cmd.get_data(size,data);

          texture2D = Texture2D::createTexture(width,height,(OSPDataType)type,data,flags);
          assert(texture2D);

          if (!(flags & OSP_TEXTURE_SHARED_BUFFER))
            free(data);

          handle.assign(texture2D);
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

        case api::MPIDevice::CMD_REMOVE_GEOMETRY: {
          const mpi::Handle modelHandle = cmd.get_handle();
          const mpi::Handle geomHandle = cmd.get_handle();
          Model *model = (Model*)modelHandle.lookup();
          Assert(model);
          Geometry *geom = (Geometry*)geomHandle.lookup();
          Assert(geom);

          GeometryLocator locator;
          locator.ptr = geom;
          Model::GeometryVector::iterator it = std::find_if(model->geometry.begin(), model->geometry.end(), locator);
          if(it != model->geometry.end()) {
            model->geometry.erase(it);
          }
        } break;

        case api::MPIDevice::CMD_ADD_VOLUME: {
          const mpi::Handle modelHandle = cmd.get_handle();
          const mpi::Handle volumeHandle = cmd.get_handle();
          Model *model = (Model *) modelHandle.lookup();
          Assert(model);
          Volume *volume = (Volume *) volumeHandle.lookup();
          Assert(volume);
          model->volumes.push_back(volume);
        } break;

        case api::MPIDevice::CMD_COMMIT: {
          const mpi::Handle handle = cmd.get_handle();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          // printf("#w%i:c%i obj %lx\n",worker.rank,(int)handle,obj);
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

        case api::MPIDevice::CMD_GET_TYPE: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();

          if (worker.rank == 0) {
            ManagedObject *object = handle.lookup();
            Assert(object);

            struct ReturnValue { int success; OSPDataType value; } result;

            if (strlen(name) == 0) {
              result.success = 1;
              result.value = object->managedObjectType;
            } else {
              ManagedObject::Param *param = object->findParam(name);
              bool foundParameter = (param != NULL);

              result.success = foundParameter ? result.value = param->type, 1 : 0;
            }

            cmd.send(&result, sizeof(ReturnValue), 0, mpi::app.comm);
            cmd.flush();
          }
        } break;

        case api::MPIDevice::CMD_GET_VALUE: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();
          OSPDataType type = (OSPDataType) cmd.get_int32();

          if (worker.rank == 0) {
            ManagedObject *object = handle.lookup();
            Assert(object);

            ManagedObject::Param *param = object->findParam(name);
            bool foundParameter = (param == NULL || param->type != type) ? false : true;

            switch (type) {
              case OSP_STRING: {
                struct ReturnValue { int success; char value[2048]; } result;
                result.success = foundParameter ? memcpy(&result.value[0], param->s, 2048), 1 : 0;
                cmd.send(&result, sizeof(ReturnValue), 0, mpi::app.comm);
              } break;
              case OSP_FLOAT: {
                struct ReturnValue { int success; float value; } result;
                result.success = foundParameter ? result.value = ((float *) param->f)[0], 1 : 0;
                cmd.send(&result, sizeof(ReturnValue), 0, mpi::app.comm);
              } break;
              case OSP_FLOAT2: {
                struct ReturnValue { int success; vec2f value; } result;
                result.success = foundParameter ? result.value = ((vec2f *) param->f)[0], 1 : 0;
                cmd.send(&result, sizeof(ReturnValue), 0, mpi::app.comm);
              } break;
              case OSP_FLOAT3: {
                struct ReturnValue { int success; vec3f value; } result;
                result.success = foundParameter ? result.value = ((vec3f *) param->f)[0], 1 : 0;
                cmd.send(&result, sizeof(ReturnValue), 0, mpi::app.comm);
              } break;
              case OSP_INT: {
                struct ReturnValue { int success; int value; } result;
                result.success = foundParameter ? result.value = ((int *) param->i)[0], 1 : 0;
                cmd.send(&result, sizeof(ReturnValue), 0, mpi::app.comm);
              } break;
              case OSP_INT3: {
                struct ReturnValue { int success; vec3i value; } result;
                result.success = foundParameter ? result.value = ((vec3i *) param->i)[0], 1 : 0;
                cmd.send(&result, sizeof(ReturnValue), 0, mpi::app.comm);
              } break;
              default: {
                throw std::runtime_error("CMD_GET_VALUE not implemented for type");
              }
            }
            cmd.flush();
          }

        } break;

        case api::MPIDevice::CMD_SET_MATERIAL: {
          const mpi::Handle geoHandle = cmd.get_handle();
          const mpi::Handle matHandle = cmd.get_handle();
          Geometry *geo = (Geometry*)geoHandle.lookup();
          Material *mat = (Material*)matHandle.lookup();
          geo->setMaterial(mat);
        } break;

        case api::MPIDevice::CMD_SET_REGION: {
          const mpi::Handle volumeHandle = cmd.get_handle();
          const mpi::Handle dataHandle = cmd.get_handle();
          const vec3i index = cmd.get_vec3i();
          const vec3i count = cmd.get_vec3i();

          Volume *volume = (Volume *)volumeHandle.lookup();
          Assert(volume);

          Data *data = (Data *)dataHandle.lookup();
          Assert(data);

          int success = volume->setRegion(data->data, index, count);

          int myFail = (success == 0);
          int sumFail = 0;
          rc = MPI_Allreduce(&myFail,&sumFail,1,MPI_INT,MPI_SUM,worker.comm);

          if (worker.rank == 0)
            MPI_Send(&sumFail,1,MPI_INT,0,0,mpi::app.comm);
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
        case api::MPIDevice::CMD_SET_VEC2F: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();
          const vec2f val = cmd.get_vec2f();
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          obj->findParam(name,1)->set(val);
          cmd.free(name);
        } break;
        case api::MPIDevice::CMD_SET_VEC2I: {
          const mpi::Handle handle = cmd.get_handle();
          const char *name = cmd.get_charPtr();
          const vec2i val = cmd.get_vec2i();
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

  } // ::ospray::api
} // ::ospray
