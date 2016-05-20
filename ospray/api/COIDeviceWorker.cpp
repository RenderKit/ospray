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

// coi
#include "COIDeviceCommon.h"
#include <sink/COIPipeline_sink.h>
#include <sink/COIProcess_sink.h>
#include <sink/COIBuffer_sink.h>
#include <common/COIMacros_common.h>
#include <common/COISysInfo_common.h>
#include <common/COIEvent_common.h>
#include "ospray/common/ObjectHandle.h"
// ospray
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/geometry/TriangleMesh.h"
#include "ospray/camera/Camera.h"
#include "ospray/volume/Volume.h"
#include "ospray/transferFunction/TransferFunction.h"
#include "ospray/render/Renderer.h"
#include "ospray/render/LoadBalancer.h"
#include "ospray/texture/Texture2D.h"
#include "ospray/lights/Light.h"
#include "ospray/fb/LocalFB.h"

using namespace std;

namespace ospray {
  extern RTCDevice g_embreeDevice;

  namespace coi {

    // only used if manual buffer uploads are turned on ...
    void *uploadBuffer = NULL;

    COINATIVELIBEXPORT
    void ospray_coi_initialize(uint32_t         in_BufferCount,
                               void**           in_ppBufferPointers,
                               uint64_t*        in_pBufferLengths,
                               void*            in_pMiscData,
                               uint16_t         in_MiscDataLength,
                               void*            in_pReturnValue,
                               uint16_t         in_ReturnValueLength)
    {
      UNREFERENCED_PARAM(in_BufferCount);
      UNREFERENCED_PARAM(in_ppBufferPointers);
      UNREFERENCED_PARAM(in_pBufferLengths);
      UNREFERENCED_PARAM(in_pMiscData);
      UNREFERENCED_PARAM(in_MiscDataLength);
      UNREFERENCED_PARAM(in_ReturnValueLength);
      UNREFERENCED_PARAM(in_pReturnValue);

      int32 *deviceInfo = (int32*)in_pMiscData;
      int deviceID = deviceInfo[0];
      int numDevices = deviceInfo[1];
      ospray::debugMode = deviceInfo[2];
      ospray::logLevel = deviceInfo[3];

      if (ospray::debugMode || ospray::logLevel >= 1) {
        std::cout << "!osp:coi: initializing device #" << deviceID
                  << " (" << (deviceID+1) << "/" << numDevices << ")"
                  << std::endl;
        COIProcessProxyFlush();
      }

      TiledLoadBalancer::instance =
          new InterleavedTiledLoadBalancer(deviceID, numDevices);
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_model(uint32_t         numBuffers,
                              void**           bufferPtr,
                              uint64_t*        bufferSize,
                              void*            argsPtr,
                              uint16_t         argsSize,
                              void*            retVal,
                              uint16_t         retValSize)
    {
      if (ospray::debugMode || ospray::logLevel >= 1) {
        std::cout << "!osp:coi: new model" << std::endl;
        COIProcessProxyFlush();
      }
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      Model *model = new Model;
      handle.assign(model);
      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_data(uint32_t         numBuffers,
                             void**           bufferPtr,
                             uint64_t*        bufferSize,
                             void*            argsPtr,
                             uint16_t         argsSize,
                             void*            retVal,
                             uint16_t         retValSize)
    {

      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      int    nitems = args.get<int32>(); 
      int    format = args.get<int32>(); 
      int    flags  = args.get<int32>(); 

      COIRESULT res = COIBufferAddRef(bufferPtr[0]);
      if (res != COI_SUCCESS) 
        throw std::runtime_error(std::string("error in coibufferaddref: ")
                                 +COIResultGetName(res));
      
      if (format == OSP_STRING)
        throw std::runtime_error("data arrays of strings not currently "
                                 "supported on coi device ...");

      if (format == OSP_OBJECT) {
        ObjectHandle *in = (ObjectHandle *)bufferPtr[0];
        ManagedObject **out = (ManagedObject **)bufferPtr[0];
        for (int i=0;i<nitems;i++) {
          if (in[i]) {
            out[i] = in[i].lookup();
            out[i]->refInc();
          } else
            out[i] = 0;
        }
        Data *data = new Data(nitems,(OSPDataType)format,
                              out,flags|OSP_DATA_SHARED_BUFFER);
        handle.assign(data);
      } else {
        Data *data = new Data(nitems,(OSPDataType)format,
                              bufferPtr[0],flags|OSP_DATA_SHARED_BUFFER);
        handle.assign(data);
      }
      
      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_upload_data_done(uint32_t         numBuffers,
                                     void**           bufferPtr,
                                     uint64_t*        bufferSize,
                                     void*            argsPtr,
                                     uint16_t         argsSize,
                                     void*            retVal,
                                     uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      int    nitems = args.get<int32>(); 
      int    format = args.get<int32>(); 
      int    flags  = args.get<int32>(); 

      if (ospray::debugMode) {
        cout << "======================================================="
             << endl;
        cout << "!osp:coi: done uploading data " << handle.ID() << endl;
      }
      
      Data *data = (Data*)handle.lookup();

      size_t size = nitems * sizeOf((OSPDataType)format);

      if (format == OSP_STRING)
        throw std::runtime_error("data arrays of strings not currently "
                                 "supported on coi device ...");

      if (format == OSP_OBJECT) {
        if (ospray::debugMode) COIProcessProxyFlush();
        ObjectHandle *in = (ObjectHandle *)data->data;
        ManagedObject **out = (ManagedObject **)data->data;
        for (int i=0;i<nitems;i++) {
          if (in[i]) {
            out[i] = in[i].lookup();
            out[i]->refInc();
          } else
            out[i] = 0;
        }
        if (ospray::debugMode) COIProcessProxyFlush();
      }
      
      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_upload_data_chunk(uint32_t         numBuffers,
                                      void**           bufferPtr,
                                      uint64_t*        bufferSize,
                                      void*            argsPtr,
                                      uint16_t         argsSize,
                                      void*            retVal,
                                      uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      int64  begin = args.get<int64>(); 
      int64  size = args.get<int64>(); 

      Data *data = (Data*)handle.lookup();
      memcpy((char*)data->data + begin, uploadBuffer, size);
      
      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_create_new_empty_data(uint32_t         numBuffers,
                                          void**           bufferPtr,
                                          uint64_t*        bufferSize,
                                          void*            argsPtr,
                                          uint16_t         argsSize,
                                          void*            retVal,
                                          uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      int    nitems = args.get<int32>(); 
      int    format = args.get<int32>(); 
      int    flags  = args.get<int32>(); 

      if (ospray::debugMode) {
        cout << "======================================================="
             << endl;
        cout << "!osp:coi: new (as-yet-empty) data " << handle.ID() << endl;
      }
      
      if (format == OSP_STRING) {
        throw std::runtime_error("data arrays of strings not currently "
                                 "supported on coi device ...");
      }

      size_t size = nitems * sizeOf((OSPDataType)format);
      void *mem = new char[size];
      bzero(mem,size);
      Data *data = new Data(nitems,(OSPDataType)format,
                            mem,flags|OSP_DATA_SHARED_BUFFER);
      handle.assign(data);
      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_geometry(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      const char *type = args.getString();

      Geometry *geom = Geometry::createGeometry(type);
      handle.assign(geom);

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_framebuffer(uint32_t         numBuffers,
                                    void**           bufferPtr,
                                    uint64_t*        bufferSize,
                                    void*            argsPtr,
                                    uint16_t         argsSize,
                                    void*            retVal,
                                    uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      vec2i size = args.get<vec2i>();
      uint32 mode = args.get<uint32>();
      uint32 channels = args.get<uint32>();

      int32 *pixelArray = (int32*)bufferPtr[0];

      FrameBuffer::ColorBufferFormat colorBufferFormat
        = (FrameBuffer::ColorBufferFormat)mode;
      bool hasDepthBuffer = (channels & OSP_FB_DEPTH)!=0;
      bool hasAccumBuffer = (channels & OSP_FB_ACCUM)!=0;
      bool hasVarianceBuffer = (channels & OSP_FB_VARIANCE)!=0;
      
      FrameBuffer *fb = new LocalFrameBuffer(size,colorBufferFormat,
                                             hasDepthBuffer,hasAccumBuffer,
                                             hasVarianceBuffer,
                                             pixelArray);
      handle.assign(fb);

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_framebuffer_clear(uint32_t         numBuffers,
                                      void**           bufferPtr,
                                      uint64_t*        bufferSize,
                                      void*            argsPtr,
                                      uint16_t         argsSize,
                                      void*            retVal,
                                      uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle _fb = args.get<ObjectHandle>();
      FrameBuffer *fb = (FrameBuffer*)_fb.lookup();
      const uint32 channel = args.get<uint32>();
      fb->clear(channel);
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_camera(uint32_t         numBuffers,
                               void**           bufferPtr,
                               uint64_t*        bufferSize,
                               void*            argsPtr,
                               uint16_t         argsSize,
                               void*            retVal,
                               uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      const char *type = args.getString();

      Camera *geom = Camera::createCamera(type);
      handle.assign(geom);

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_volume(uint32_t         numBuffers,
                               void**           bufferPtr,
                               uint64_t*        bufferSize,
                               void*            argsPtr,
                               uint16_t         argsSize,
                               void*            retVal,
                               uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      const char *type = args.getString();
      if (ospray::logLevel > 0)
        cout << "!osp:coi: new volume " << handle.ID() << " " << type << endl;

      Volume *geom = Volume::createInstance(type);
      handle.assign(geom);

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_transfer_function(uint32_t         numBuffers,
                                          void**           bufferPtr,
                                          uint64_t*        bufferSize,
                                          void*            argsPtr,
                                          uint16_t         argsSize,
                                          void*            retVal,
                                          uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      const char *type = args.getString();
      cout << "!osp:coi: new transfer function " << handle.ID()
           << " " << type << endl;

      TransferFunction *transferFunction =
          TransferFunction::createInstance(type);
      handle.assign(transferFunction);

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_renderer(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      const char *type = args.getString();

      Renderer *geom = Renderer::createRenderer(type);
      handle.assign(geom);

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_pin_upload_buffer(uint32_t         numBuffers,
                                      void**           bufferPtr,
                                      uint64_t*        bufferSize,
                                      void*            argsPtr,
                                      uint16_t         argsSize,
                                      void*            retVal,
                                      uint16_t         retValSize)
    {
      void *buffer = bufferPtr[0];
      uploadBuffer = buffer;
      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_material(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      ObjectHandle _renderer = args.get<ObjectHandle>();
      const char *type = args.getString();

      Renderer *renderer = (Renderer*)_renderer.lookup();
      Material *mat = NULL;
      if (renderer)
        mat = renderer->createMaterial(type);
      if (!mat)
        mat = Material::createMaterial(type);
      
      if (mat) {
        *(int*)retVal = true;
        handle.assign(mat);
      } else {
        *(int*)retVal = false;
      }

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_pick(uint32_t         numBuffers,
                         void**           bufferPtr,
                         uint64_t*        bufferSize,
                         void*            argsPtr,
                         uint16_t         argsSize,
                         void*            retVal,
                         uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle _renderer = args.get<ObjectHandle>();
      vec2f screenPos = args.get<vec2f>();

      Renderer *renderer = (Renderer *)_renderer.lookup();
      OSPPickResult retData = renderer->pick(screenPos);
      memcpy(retVal, &retData, retValSize);

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_sample_volume(uint32_t         numBuffers,
                                  void**           bufferPtr,
                                  uint64_t*        bufferSize,
                                  void*            argsPtr,
                                  uint16_t         argsSize,
                                  void*            retVal,
                                  uint16_t         retValSize)
    {
      DataStream args(argsPtr);

      ObjectHandle _volume = args.get<ObjectHandle>();
      Volume *volume = (Volume *)_volume.lookup();
      Assert(volume);

      ObjectHandle _worldCoordinatesData = args.get<ObjectHandle>();
      Data *worldCoordinatesData = (Data *)_worldCoordinatesData.lookup();
      Assert(worldCoordinatesData);

      float *results = NULL;
      volume->computeSamples(&results,
                             (const vec3f *)worldCoordinatesData->data,
                             worldCoordinatesData->numItems);
      Assert(results);

      memcpy(retVal, results, retValSize);
      free(results);

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_light(uint32_t         numBuffers,
                              void**           bufferPtr,
                              uint64_t*        bufferSize,
                              void*            argsPtr,
                              uint16_t         argsSize,
                              void*            retVal,
                              uint16_t         retValSize)
    {
      if (ospray::logLevel >= 2) {
        std::cout << "!osp:coi: new light" << std::endl;
        COIProcessProxyFlush();
      }
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      ObjectHandle _renderer = args.get<ObjectHandle>();
      const char *type = args.getString();
      Renderer *renderer = (Renderer *)_renderer.lookup();

      Light *light = NULL;
      if (ospray::debugMode) COIProcessProxyFlush(); 

      if (renderer)
        light = renderer->createLight(type);
      if (!light)
        light = Light::createLight(type);
      assert(light);
      handle.assign(light);

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_new_texture2d(uint32_t         numBuffers,
                                  void**           bufferPtr,
                                  uint64_t*        bufferSize,
                                  void*            argsPtr,
                                  uint16_t         argsSize,
                                  void*            retVal,
                                  uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();
      vec2i  size   = args.get<vec2i>();
      int    type   = args.get<int32>(); 
      int    flags  = args.get<int32>(); 

      COIBufferAddRef(bufferPtr[0]);

      Texture2D *tx = Texture2D::createTexture(size,
                                               (OSPTextureFormat)type,
                                               bufferPtr[0],
                                               flags);

      handle.assign(tx);
      if (ospray::debugMode) COIProcessProxyFlush();
    }
                                
    COINATIVELIBEXPORT
    void ospray_coi_add_geometry(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle model = args.get<ObjectHandle>();
      ObjectHandle geom  = args.get<ObjectHandle>();

      Model *m = (Model*)model.lookup();
      m->geometry.push_back((Geometry*)geom.lookup());
      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_add_volume(uint32_t         numBuffers,
                               void**           bufferPtr,
                               uint64_t*        bufferSize,
                               void*            argsPtr,
                               uint16_t         argsSize,
                               void*            retVal,
                               uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle model = args.get<ObjectHandle>();
      ObjectHandle volume = args.get<ObjectHandle>();

      Model *m = (Model *) model.lookup();
      m->volume.push_back((Volume *) volume.lookup());
      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_set_material(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle geom = args.get<ObjectHandle>();
      ObjectHandle mat  = args.get<ObjectHandle>();

      Geometry *g = (Geometry*)geom.lookup();
      g->setMaterial((Material*)mat.lookup());
      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_commit(uint32_t         numBuffers,
                           void**           bufferPtr,
                           uint64_t*        bufferSize,
                           void*            argsPtr,
                           uint16_t         argsSize,
                           void*            retVal,
                           uint16_t         retValSize)
    {
      if (ospray::debugMode || ospray::logLevel >= 1) {
        std::cout << "!osp:coi: commit" << std::endl;
        COIProcessProxyFlush();
      }
      DataStream args(argsPtr);
      ObjectHandle handle = args.get<ObjectHandle>();

      ManagedObject *obj = handle.lookup();
      Assert(obj);
      obj->commit();


      // hack, to stay compatible with earlier version
      Model *model = dynamic_cast<Model *>(obj);
      if (model) {
        model->finalize();
      }

      if (ospray::debugMode) COIProcessProxyFlush();
      if (ospray::debugMode || ospray::logLevel >= 1) {
        std::cout << "!osp:coi: DONE commit" << std::endl;
        COIProcessProxyFlush();
      }
    }

    COINATIVELIBEXPORT
    void ospray_coi_release(uint32_t         numBuffers,
                            void**           bufferPtr,
                            uint64_t*        bufferSize,
                            void*            argsPtr,
                            uint16_t         argsSize,
                            void*            retVal,
                            uint16_t         retValSize)
    {

      DataStream stream(argsPtr);
      ObjectHandle handle = stream.get<ObjectHandle>();
      ManagedObject *object = handle.lookup();
      object->refDec();
      handle.freeObject();
      if (ospray::debugMode) COIProcessProxyFlush();

    }

    /*! remove an existing geometry from a model */
    struct GeometryLocator {
      bool operator()(const Ref<ospray::Geometry> &g) const {
        return ptr == &*g;
      }
      Geometry *ptr;
    };

    COINATIVELIBEXPORT
    void ospray_coi_remove_geometry(uint32_t         numBuffers,
                                    void**           bufferPtr,
                                    uint64_t*        bufferSize,
                                    void*            argsPtr,
                                    uint16_t         argsSize,
                                    void*            retVal,
                                    uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle _model = args.get<ObjectHandle>();
      ObjectHandle _geometry = args.get<ObjectHandle>();

      Model *model = (Model*)_model.lookup();
      Geometry *geometry = (Geometry*)_geometry.lookup();

      GeometryLocator locator;
      locator.ptr = geometry;
      Model::GeometryVector::iterator it =
          std::find_if(model->geometry.begin(),
                       model->geometry.end(), locator);
      if(it != model->geometry.end()) {
        model->geometry.erase(it);
      }

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    /*! remove an existing volume from a model */
    struct VolumeLocator {
      bool operator()(const Ref<ospray::Volume> &g) const {
        return ptr == &*g;
      }
      Volume *ptr;
    };

    COINATIVELIBEXPORT
    void ospray_coi_remove_volume(uint32_t         numBuffers,
                                  void**           bufferPtr,
                                  uint64_t*        bufferSize,
                                  void*            argsPtr,
                                  uint16_t         argsSize,
                                  void*            retVal,
                                  uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle _model = args.get<ObjectHandle>();
      ObjectHandle _volume = args.get<ObjectHandle>();

      Model *model = (Model*)_model.lookup();
      Volume *volume = (Volume*)_volume.lookup();

      VolumeLocator locator;
      locator.ptr = volume;
      Model::VolumeVector::iterator it =
          std::find_if(model->volume.begin(), model->volume.end(), locator);
      if(it != model->volume.end()) {
        model->volume.erase(it);
      }

      if (ospray::debugMode) COIProcessProxyFlush();
    }

    COINATIVELIBEXPORT
    void ospray_coi_render_frame(uint32_t         numBuffers,
                                 void**           bufferPtr,
                                 uint64_t*        bufferSize,
                                 void*            argsPtr,
                                 uint16_t         argsSize,
                                 void*            retVal,
                                 uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle _fb      = args.get<ObjectHandle>();
      ObjectHandle renderer = args.get<ObjectHandle>();
      uint32 channelFlags = args.get<uint32>();
      FrameBuffer *fb = (FrameBuffer*)_fb.lookup();
      Renderer *r = (Renderer*)renderer.lookup();
      float v = r->renderFrame(fb, channelFlags);
      memcpy(retVal, &v, retValSize);
    }

    COINATIVELIBEXPORT
    void ospray_coi_render_frame_sync(uint32_t         numBuffers,
                                      void**           bufferPtr,
                                      uint64_t*        bufferSize,
                                      void*            argsPtr,
                                      uint16_t         argsSize,
                                      void*            retVal,
                                      uint16_t         retValSize)
    {
      // currently all rendering is synchronous, anyway ....
    }

    COINATIVELIBEXPORT
    void ospray_coi_set_region(uint32_t         numBuffers,
                               void**           bufferPtr,
                               uint64_t*        bufferSize,
                               void*            argsPtr,
                               uint16_t         argsSize,
                               void*            retVal,
                               uint16_t         retValSize)
    {

      DataStream stream(argsPtr);
      Volume *volume = (Volume *) stream.get<ObjectHandle>().lookup();
      Data *data  = (Data *) stream.get<ObjectHandle>().lookup();
      vec3i index = stream.get<vec3i>();
      vec3i count = stream.get<vec3i>();
      int *result = (int *) retVal;
      result[0] = volume->setRegion(data->data, index, count);

    }

    COINATIVELIBEXPORT
    void ospray_coi_set_value(uint32_t         numBuffers,
                              void**           bufferPtr,
                              uint64_t*        bufferSize,
                              void*            argsPtr,
                              uint16_t         argsSize,
                              void*            retVal,
                              uint16_t         retValSize)
    {
      DataStream args(argsPtr);
      ObjectHandle target = args.get<ObjectHandle>();
      const char *name = args.getString();
      ManagedObject *obj = target.lookup();
      if (!obj) return;
    
      Assert(obj);

      OSPDataType type = args.get<OSPDataType>();

      switch (type) {
      case OSP_INT : obj->set(name,args.get<int32>()); break;
      case OSP_INT2: obj->set(name,args.get<vec2i>()); break;
      case OSP_INT3: obj->set(name,args.get<vec3i>()); break;
      case OSP_INT4: obj->set(name,args.get<vec4i>()); break;

      case OSP_UINT : obj->set(name,args.get<uint32>()); break;
      case OSP_UINT2: obj->set(name,args.get<vec2ui>()); break;
      case OSP_UINT3: obj->set(name,args.get<vec3ui>()); break;
      case OSP_UINT4: obj->set(name,args.get<vec4ui>()); break;

      case OSP_FLOAT : obj->set(name,args.get<float>()); break;
      case OSP_FLOAT2: obj->set(name,args.get<vec2f>()); break;
      case OSP_FLOAT3: obj->set(name,args.get<vec3f>()); break;
      case OSP_FLOAT4: obj->set(name,args.get<vec4f>()); break;
        
      case OSP_STRING: obj->set(name,args.getString()); break;
      case OSP_OBJECT: obj->set(name,args.get<ObjectHandle>().lookup()); break;

      default:
        throw "ospray_coi_set_value no timplemented for given data type";
      }
    }

    COINATIVELIBEXPORT
    void ospray_coi_get_data_values(uint32_t         numBuffers,
                                    void**           bufferPtr,
                                    uint64_t*        bufferSize,
                                    void*            argsPtr,
                                    uint16_t         argsSize,
                                    void*            retVal,
                                    uint16_t         retValSize)
    {

      DataStream stream(argsPtr);
      Data *data = (Data *) stream.get<ObjectHandle>().lookup();
      int *result = (int *) retVal;  result[0] = false;
      if (bufferSize[0] != data->numBytes) return;
      memcpy(bufferPtr[0], data->data, data->numBytes);  result[0] = true;

    }

    COINATIVELIBEXPORT
    void ospray_coi_get_data_properties(uint32_t         numBuffers,
                                        void**           bufferPtr,
                                        uint64_t*        bufferSize,
                                        void*            argsPtr,
                                        uint16_t         argsSize,
                                        void*            retVal,
                                        uint16_t         retValSize)
    {

      DataStream stream(argsPtr);
      Data *data = (Data *) stream.get<ObjectHandle>().lookup();
      int *result = (int *) retVal;  result[0] = true;
      memcpy(&result[1], &data->numItems, sizeof(size_t));
      memcpy((char *)(&result[1]) + sizeof(size_t),
             &data->type, sizeof(OSPDataType));

    }

    COINATIVELIBEXPORT
    void ospray_coi_get_parameters(uint32_t         numBuffers,
                                   void**           bufferPtr,
                                   uint64_t*        bufferSize,
                                   void*            argsPtr,
                                   uint16_t         argsSize,
                                   void*            retVal,
                                   uint16_t         retValSize)
    {

      //! The size of ReturnValue is larger than that indicated by the
      //  following definition.
      DataStream stream(argsPtr);
      ManagedObject *object = stream.get<ObjectHandle>().lookup();
      int *result = (int *) retVal;  result[0] = true;

      for (size_t i=0, offset=0 ; i < object->paramList.size() ; i++) {

          size_t size = strlen(object->paramList[i]->name) + 1;
          memcpy((char *)(&result[1]) + offset,
                 object->paramList[i]->name, size);
          offset += size;

      }

    }

    COINATIVELIBEXPORT
    void ospray_coi_get_parameters_size(uint32_t         numBuffers,
                                        void**           bufferPtr,
                                        uint64_t*        bufferSize,
                                        void*            argsPtr,
                                        uint16_t         argsSize,
                                        void*            retVal,
                                        uint16_t         retValSize)
    {

      DataStream stream(argsPtr);
      ManagedObject *object = stream.get<ObjectHandle>().lookup();
      int *result = (int *) retVal;  result[0] = true;

      int size = 0;
      for (size_t i = 0; i < object->paramList.size(); i++)
        size += (strlen(object->paramList[i]->name) + 1) * sizeof(char);
      memcpy(&result[1], &size, sizeof(int));

    }

    COINATIVELIBEXPORT
    void ospray_coi_get_type(uint32_t         numBuffers,
                             void**           bufferPtr,
                             uint64_t*        bufferSize,
                             void*            argsPtr,
                             uint16_t         argsSize,
                             void*            retVal,
                             uint16_t         retValSize)
    {

      DataStream stream(argsPtr);
      ManagedObject *object = stream.get<ObjectHandle>().lookup();
      const char *name = stream.getString();
      int *result = (int *) retVal;  result[0] = false;

      switch (strlen(name)) {
        case 0: {

          OSPDataType type = object->managedObjectType;
          result[0] = true;
          memcpy(&result[1], &type, sizeof(OSPDataType));
          break;

        } default: {

          ManagedObject::Param *param = object->findParam(name);
          if (param == NULL) return;
          OSPDataType type =
              (param->type == OSP_OBJECT) ? param->ptr->managedObjectType :
                                            param->type;
          result[0] = true;
          memcpy(&result[1], &type, sizeof(OSPDataType));
          break;

        }
      }
    }

    COINATIVELIBEXPORT
    void ospray_coi_get_value(uint32_t         numBuffers,
                              void**           bufferPtr,
                              uint64_t*        bufferSize,
                              void*            argsPtr,
                              uint16_t         argsSize,
                              void*            retVal,
                              uint16_t         retValSize)
    {

      DataStream stream(argsPtr);
      ManagedObject *object = stream.get<ObjectHandle>().lookup();
      const char *name = stream.getString();
      OSPDataType type = stream.get<OSPDataType>();
      ManagedObject::Param *param = object->findParam(name);
      int *result = (int *) retVal;  result[0] = false;

      switch (type) {
        case OSP_DATA: {

          if (param == NULL ||
              param->type != OSP_OBJECT ||
              param->ptr->managedObjectType != OSP_DATA) return;
          ObjectHandle handle = ObjectHandle::lookup(param->ptr);
          result[0] = true;
          memcpy(&result[1], &handle, sizeof(ObjectHandle));
          break;

        } case OSP_FLOAT: {

          if (param == NULL || param->type != OSP_FLOAT) return;
          float value = param->f[0];
          result[0] = true;  memcpy(&result[1], &value, sizeof(float));  break;

        } case OSP_FLOAT2: {

          if (param == NULL || param->type != OSP_FLOAT2) return;
          vec2f value = ((vec2f *) param->f)[0];
          result[0] = true;  memcpy(&result[1], &value, sizeof(vec2f));  break;

        } case OSP_FLOAT3: {

          if (param == NULL || param->type != OSP_FLOAT3) return;
          vec3f value = ((vec3f *) param->f)[0];
          result[0] = true;  memcpy(&result[1], &value, sizeof(vec3f));  break;

        } case OSP_FLOAT4: {

          if (param == NULL || param->type != OSP_FLOAT4) return;
          vec4f value = ((vec4f *) param->f)[0];
          result[0] = true;  memcpy(&result[1], &value, sizeof(vec4f));  break;

        } case OSP_INT: {

          if (param == NULL || param->type != OSP_INT) return;
          int value = param->i[0];
          result[0] = true;  memcpy(&result[1], &value, sizeof(int));  break;

        } case OSP_INT3: {

          if (param == NULL || param->type != OSP_INT3) return;
          vec3i value = ((vec3i *) param->i)[0];
          result[0] = true;  memcpy(&result[1], &value, sizeof(vec3i));  break;

        } case OSP_MATERIAL: {

          if (object->managedObjectType != OSP_GEOMETRY ||
              ((Geometry *) object)->getMaterial() == NULL) return;
          ObjectHandle handle =
              ObjectHandle::lookup(((Geometry *) object)->getMaterial());
          result[0] = true;
          memcpy(&result[1], &handle, sizeof(ObjectHandle));
          break;

        } case OSP_OBJECT: {

          if (param == NULL || param->type != OSP_OBJECT) return;
          ObjectHandle handle = ObjectHandle::lookup(param->ptr);
          result[0] = true;
          memcpy(&result[1], &handle, sizeof(ObjectHandle));
          break;

        } case OSP_STRING: {

          if (param == NULL || param->type != OSP_STRING) return;
          const char *value = param->s;
          result[0] = true;
          memcpy(&result[1], value, strnlen(value, retValSize - 1) + 1);
          break;

        } default:  return;

      }
    }

  } // ::ospray::coi
} // ::ospray

void embreeErrorFunc(const RTCError code, const char* str)
{
  std::cerr << "#osp: embree internal error " << code << " : " << str
            << std::endl;
  throw std::runtime_error("embree internal error '" + std::string(str) + "'");
}

int main(int ac, const char **av)
{
  ospray::init(&ac,&av);
  if (ospray::logLevel >= 1)
    printf("!osp:coi: ospray_coi_worker starting up.\n");

  // initialize embree. (we need to do this here rather than in
  // ospray::init() because in mpi-mode the latter is also called
  // in the host-stubs, where it shouldn't.
  std::stringstream embreeConfig;
  embreeConfig << "verbose=" << ospray::logLevel;
  if (ospray::debugMode) 
    embreeConfig << " threads=1,verbose=2";
  else if(ospray::numThreads > 0)
    embreeConfig << " threads=" << ospray::numThreads;
  ospray::g_embreeDevice = rtcNewDevice(embreeConfig.str().c_str());

  rtcDeviceSetErrorFunction(ospray::g_embreeDevice, embreeErrorFunc);
  RTCError erc = rtcDeviceGetError(ospray::g_embreeDevice);
  if (erc != RTC_NO_ERROR) {
    // why did the error function not get called !?
    std::cerr << "#osp:init: embree internal error number " << (int)erc
              << std::endl;
    assert(erc == RTC_NO_ERROR);
  }

  ospray::TiledLoadBalancer::instance = NULL;

  COIPipelineStartExecutingRunFunctions();
  if (ospray::debugMode) COIProcessProxyFlush();
  COIProcessWaitForShutdown();
}
