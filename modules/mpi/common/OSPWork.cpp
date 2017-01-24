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

#include <vector>

#include "mpiCommon/MPICommon.h"
#include "OSPWork.h"
#include "ospray/common/ObjectHandle.h"
#include "mpi/fb/DistributedFrameBuffer.h"
#include "mpi/render/MPILoadBalancer.h"

#include "common/Model.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/Model.h"
#include "geometry/TriangleMesh.h"
#include "render/Renderer.h"
#include "camera/Camera.h"
#include "volume/Volume.h"
#include "lights/Light.h"
#include "texture/Texture2D.h"
#include "transferFunction/TransferFunction.h"

namespace ospray {
  namespace mpi {
    namespace work {

      const char* commandToString(CommandTag tag) {
        switch (tag) {
        case CMD_INVALID: return "CMD_INVALID";
        case CMD_NEW_RENDERER: return "CMD_NEW_RENDERER";
        case CMD_FRAMEBUFFER_CREATE: return "CMD_FRAMEBUFFER_CREATE";
        case CMD_RENDER_FRAME: return "CMD_RENDER_FRAME";
        case CMD_FRAMEBUFFER_CLEAR: return "CMD_FRAMEBUFFER_CLEAR";
        case CMD_FRAMEBUFFER_MAP: return "CMD_FRAMEBUFFER_MAP";
        case CMD_FRAMEBUFFER_UNMAP: return "CMD_FRAMEBUFFER_UNMAP";
        case CMD_NEW_DATA: return "CMD_NEW_DATA";
        case CMD_NEW_MODEL: return "CMD_NEW_MODEL";
        case CMD_NEW_GEOMETRY: return "CMD_NEW_GEOMETRY";
        case CMD_NEW_MATERIAL: return "CMD_NEW_MATERIAL";
        case CMD_NEW_LIGHT: return "CMD_NEW_LIGHT";
        case CMD_NEW_CAMERA: return "CMD_NEW_CAMERA";
        case CMD_NEW_VOLUME: return "CMD_NEW_VOLUME";
        case CMD_NEW_TRANSFERFUNCTION: return "CMD_NEW_TRANSFERFUNCTION";
        case CMD_NEW_TEXTURE2D: return "CMD_NEW_TEXTURE2D";
        case CMD_ADD_GEOMETRY: return "CMD_ADD_GEOMETRY";
        case CMD_REMOVE_GEOMETRY: return "CMD_REMOVE_GEOMETRY";
        case CMD_ADD_VOLUME: return "CMD_ADD_VOLUME";
        case CMD_COMMIT: return "CMD_COMMIT";
        case CMD_LOAD_MODULE: return "CMD_LOAD_MODULE";
        case CMD_RELEASE: return "CMD_RELEASE";
        case CMD_REMOVE_VOLUME: return "CMD_REMOVE_VOLUME";
        case CMD_SET_MATERIAL: return "CMD_SET_MATERIAL";
        case CMD_SET_REGION: return "CMD_SET_REGION";
        case CMD_SET_REGION_DATA: return "CMD_SET_REGION_DATA";
        case CMD_SET_OBJECT: return "CMD_SET_OBJECT";
        case CMD_SET_STRING: return "CMD_SET_STRING";
        case CMD_SET_INT: return "CMD_SET_INT";
        case CMD_SET_FLOAT: return "CMD_SET_FLOAT";
        case CMD_SET_VEC2F: return "CMD_SET_VEC2F";
        case CMD_SET_VEC2I: return "CMD_SET_VEC2I";
        case CMD_SET_VEC3F: return "CMD_SET_VEC3F";
        case CMD_SET_VEC3I: return "CMD_SET_VEC3I";
        case CMD_SET_VEC4F: return "CMD_SET_VEC4F";
        case CMD_SET_PIXELOP: return "CMD_SET_PIXELOP";
        case CMD_REMOVE_PARAM: return "CMD_REMOVE_PARAM";
        case CMD_NEW_PIXELOP: return "CMD_NEW_PIXELOP";
        case CMD_API_MODE: return "CMD_API_MODE";
        case CMD_FINALIZE: return "CMD_FINALIZE";
        default: return "Unrecognized CommandTag";
        }
      }


      // =======================================================
      // CMD_COMMIT
      // =======================================================
      
      CommitObject::CommitObject()
      {}
      
      CommitObject::CommitObject(ObjectHandle handle)
        : handle(handle)
      {}
      
      void CommitObject::run()
      {
        ManagedObject *obj = handle.lookup();
        if (obj) {
          obj->commit();

          // TODO: Do we need this hack anymore?
          // It looks like yes? or at least glutViewer segfaults if we don't do this
          // hack, to stay compatible with earlier version
          Model *model = dynamic_cast<Model*>(obj);
          if (model) {
            model->finalize();
          }
        } else {
          throw std::runtime_error("Error: rank " + std::to_string(mpi::world.rank)
                                   + " did not have object to commit!");
        }
        // TODO: Work units should not be directly making MPI calls.
        // What should be responsible for this barrier?
        // MPI_Barrier(MPI_COMM_WORLD);
        mpi::world.barrier();
      }
      
      void CommitObject::runOnMaster()
      {
        ManagedObject *obj = handle.lookup();
        if (dynamic_cast<Renderer*>(obj)) {
          obj->commit();
        }
        mpi::world.barrier();
      }
      
      void CommitObject::serialize(WriteStream &b) const
      {
        b << (int64)handle;
      }
      
      void CommitObject::deserialize(ReadStream &b)
      {
        b >> handle.i64;
      }

      // =======================================================
      // CMD_CREATE_FRAMEBUFFER
      // =======================================================

      CreateFrameBuffer::CreateFrameBuffer()
      {}
      
      CreateFrameBuffer::CreateFrameBuffer(ObjectHandle handle,
                                           vec2i dimensions,
                                           OSPFrameBufferFormat format,
                                           uint32 channels)
        : handle(handle),
          dimensions(dimensions),
          format(format),
          channels(channels)
      {}
    
      void CreateFrameBuffer::run()
      {
        const bool hasDepthBuffer = channels & OSP_FB_DEPTH;
        const bool hasAccumBuffer = channels & OSP_FB_ACCUM;
        const bool hasVarianceBuffer = channels & OSP_FB_VARIANCE;
        FrameBuffer *fb = new DistributedFrameBuffer(ospray::mpi::async::CommLayer::WORLD,
                                                     dimensions, handle, format, hasDepthBuffer, hasAccumBuffer, hasVarianceBuffer);

        // TODO: Only the master does this increment, though should the workers do it too?
        fb->refInc();
        handle.assign(fb);
      }
      
      void CreateFrameBuffer::runOnMaster()
      {
        run();
      }
      
      void CreateFrameBuffer::serialize(WriteStream &b) const
      {
        b << (int64)handle << dimensions << (int32)format << channels;
      }
      
      void CreateFrameBuffer::deserialize(ReadStream &b)
      {
        int32 fmt;
        b >> handle.i64 >> dimensions >> fmt >> channels;
        format = (OSPFrameBufferFormat)fmt;
      }

      // =======================================================
      // CMD_LOAD_MODULE
      // =======================================================
      
      LoadModule::LoadModule()
      {}
      
      LoadModule::LoadModule(const std::string &name)
        : name(name)
      {}
      
      void LoadModule::run()
      {
        const std::string libName = "ospray_module_" + name;
        loadLibrary(libName);

        const std::string initSymName = "ospray_init_module_" + name;
        void *initSym = getSymbol(initSymName);
        if (!initSym) {
          throw std::runtime_error("could not find module initializer " + initSymName);
        }
        void (*initMethod)() = (void(*)())initSym;
        initMethod();
      }
      
      void LoadModule::serialize(WriteStream &b) const
      {
        b << name;
      }
      
      void LoadModule::deserialize(ReadStream &b)
      {
        b >> name;
      }

      // =======================================================
      // CMD_SET_PARAM<...>
      // =======================================================
      template<>
      void SetParam<std::string>::run() {
        ManagedObject *obj = handle.lookup();
        Assert(obj);
        obj->findParam(name.c_str(), true)->set(val.c_str());
      }
    
      template<>
      void SetParam<std::string>::runOnMaster() {
        ManagedObject *obj = handle.lookup();
        if (dynamic_cast<Renderer*>(obj) || dynamic_cast<Volume*>(obj)) {
          obj->findParam(name.c_str(), true)->set(val.c_str());
        }
      }


      // =======================================================
      // CMD_SET_MATERIAL
      // =======================================================
      void SetMaterial::run() 
      {
        Geometry *geom = (Geometry*)handle.lookup();
        Material *mat = (Material*)material.lookup();
        Assert(geom);
        Assert(mat);
        /* might we worthwhile doing a dyncast here to check if that
           is actually a proper geometry .. */
        geom->setMaterial(mat);
      }

      
      template<typename T>
      void SetParam<T>::run() 
      {
        ManagedObject *obj = handle.lookup();
        Assert(obj);
        obj->findParam(name.c_str(), true)->set(val);
      }
      
      template<typename T>
      void SetParam<T>::runOnMaster() 
      {
        ManagedObject *obj = handle.lookup();
        if (dynamic_cast<Renderer*>(obj) || dynamic_cast<Volume*>(obj)) {
          obj->findParam(name.c_str(), true)->set(val);
        }
      }


      
      /*! create a work unit of given type */
      template<typename T>
      inline std::shared_ptr<Work> make_work_unit()
      {
        return std::make_shared<T>();
      }

      template<typename T>
      CreateWorkFct createMakeWorkFct()
      { return make_work_unit<T>; }
      
#define REGISTER_WORK_UNIT(W) workTypeRegistry[W::tag] = createMakeWorkFct<W>();
      
      void registerOSPWorkItems(std::map<Work::tag_t,CreateWorkFct> &workTypeRegistry)
      {
        REGISTER_WORK_UNIT(NewRenderer);
        REGISTER_WORK_UNIT(NewModel);
        REGISTER_WORK_UNIT(NewGeometry);
        REGISTER_WORK_UNIT(NewCamera);
        REGISTER_WORK_UNIT(NewVolume);
        REGISTER_WORK_UNIT(NewTransferFunction);
        REGISTER_WORK_UNIT(NewPixelOp);

        REGISTER_WORK_UNIT(NewMaterial);
        REGISTER_WORK_UNIT(NewLight);

        REGISTER_WORK_UNIT(NewData);
        REGISTER_WORK_UNIT(NewTexture2d);

        REGISTER_WORK_UNIT(CommitObject);
        REGISTER_WORK_UNIT(CommandRelease);

        REGISTER_WORK_UNIT(LoadModule);

        REGISTER_WORK_UNIT(AddGeometry);
        REGISTER_WORK_UNIT(AddVolume);
        REGISTER_WORK_UNIT(RemoveGeometry);
        REGISTER_WORK_UNIT(RemoveVolume);

        REGISTER_WORK_UNIT(CreateFrameBuffer);
        REGISTER_WORK_UNIT(ClearFrameBuffer);
        REGISTER_WORK_UNIT(RenderFrame);

        REGISTER_WORK_UNIT(SetRegion);
        REGISTER_WORK_UNIT(SetPixelOp);

        REGISTER_WORK_UNIT(SetMaterial);
        REGISTER_WORK_UNIT(SetParam<OSPObject>);
        REGISTER_WORK_UNIT(SetParam<std::string>);
        REGISTER_WORK_UNIT(SetParam<int>);
        REGISTER_WORK_UNIT(SetParam<float>);
        REGISTER_WORK_UNIT(SetParam<vec2f>);
        REGISTER_WORK_UNIT(SetParam<vec2i>);
        REGISTER_WORK_UNIT(SetParam<vec3f>);
        REGISTER_WORK_UNIT(SetParam<vec3i>);
        REGISTER_WORK_UNIT(SetParam<vec4f>);

        REGISTER_WORK_UNIT(RemoveParam);

        REGISTER_WORK_UNIT(CommandFinalize);
      }
#undef REGISTER_WORK_UNIT


      template<>
      void NewObjectT<ObjectType_Renderer>::run()
      {
        Renderer *renderer = Renderer::createRenderer(type.c_str());
        if (!renderer) {
          throw std::runtime_error("unknown renderer type '" + type + "'");
        }
        handle.assign(renderer);
      }
      
      template<>
      void NewRenderer::runOnMaster()
      {
        run();
      }
      
      template<>
      void NewModel::run()
      {
        Model *model = new Model;
        handle.assign(model);
      }
      
      template<>
      void NewGeometry::run()
      {
        Geometry *geometry = Geometry::createGeometry(type.c_str());
        if (!geometry) {
          throw std::runtime_error("unknown geometry type '" + type + "'");
        }
        // TODO: Why is this manual reference increment needed!?
        // is it to keep the object alive until ospRelease is called
        // since in the distributed mode no app has a reference to this object?
        geometry->refInc();
        handle.assign(geometry);
      }
      
      template<>
      void NewCamera::run()
      {
        Camera *camera = Camera::createCamera(type.c_str());
        Assert(camera);
        handle.assign(camera);
      }
      
      template<>
      void NewVolume::run()
      {
        Volume *volume = Volume::createInstance(type.c_str());
        if (!volume) {
          throw std::runtime_error("unknown volume type '" + type + "'");
        }
        volume->refInc();
        handle.assign(volume);
      }
      
      template<>
      void NewVolume::runOnMaster()
      {
        run();
      }
      
      template<>
      void NewTransferFunction::run()
      {
        TransferFunction *tfn = TransferFunction::createInstance(type.c_str());
        if (!tfn) {
          throw std::runtime_error("unknown transfer functon type '" + type + "'");
        }
        tfn->refInc();
        handle.assign(tfn);
      }
      template<>
      void NewPixelOp::run() {
        PixelOp *pixelOp = PixelOp::createPixelOp(type.c_str());
        if (!pixelOp) {
          throw std::runtime_error("unknown pixel op type '" + type + "'");
        }
        handle.assign(pixelOp);
      }

      void NewMaterial::run()
      {
        Renderer *renderer = (Renderer*)rendererHandle.lookup();
        Material *material = nullptr;
        if (renderer) {
          material = renderer->createMaterial(type.c_str());
          if (material) {
            material->refInc();
          }
        }
        // No renderer present or the renderer doesn't intercept this
        // material type.
        if (!material) {
          material = Material::createMaterial(type.c_str());
        }
        if (!material) {
          throw std::runtime_error("unknown material type '" + type + "'");
        }
        handle.assign(material);
      }
      
      void NewLight::run()
      {
        Renderer *renderer = (Renderer*)rendererHandle.lookup();
        Light *light = nullptr;
        if (renderer) {
          light = renderer->createLight(type.c_str());
          if (light) {
            light->refInc();
          }
        }
        // No renderer present or the renderer doesn't intercept this
        // material type.
        if (!light) {
          light = Light::createLight(type.c_str());
        }
        if (!light) {
          throw std::runtime_error("unknown light type '" + type + "'");
        }
        handle.assign(light);
      }

      NewData::NewData()
      {}
      
      NewData::NewData(ObjectHandle handle, size_t nItems,
                       OSPDataType format, void *init, int flags)
        : handle(handle),
          nItems(nItems),
          format(format),
          localData(nullptr),
          flags(flags)
      {
        // TODO: Is this check ok for ParaView e.g. what Carson is changing in 2e81c005 ?
        if (init && nItems) {
          if (flags & OSP_DATA_SHARED_BUFFER) {
            localData = init;
          } else {
            data.resize(ospray::sizeOf(format) * nItems);
            std::memcpy(data.data(), init, data.size());
          }
        }
      }
      
      void NewData::run()
      {
        Data *ospdata = nullptr;
        if (!data.empty()) {
          ospdata = new Data(nItems, format, data.data(), flags);
        } else if (localData) {
          ospdata = new Data(nItems, format, localData, flags);
        } else {
          ospdata = new Data(nItems, format, nullptr, flags);
        }
        Assert(ospdata);
        handle.assign(ospdata);
        if (format == OSP_OBJECT) {
          /* translating handles to managedobject pointers: if a
             data array has 'object' or 'data' entry types, then
             what the host sends are _handles_, not pointers, but
             what the core expects are pointers; to make the core
             happy we translate all data items back to pointers at
             this stage */
          ObjectHandle *asHandle = (ObjectHandle*)ospdata->data;
          ManagedObject **asObjPtr = (ManagedObject**)ospdata->data;
          for (size_t i = 0; i < nItems; ++i) {
            if (asHandle[i] != NULL_HANDLE) {
              asObjPtr[i] = asHandle[i].lookup();
              asObjPtr[i]->refInc();
            }
          }
        }
      }
      
      void NewData::serialize(WriteStream &b) const
      {
        b << (int64)handle << nItems << (int32)format << flags << data;
      }
      
      void NewData::deserialize(ReadStream &b)
      {
        int32 fmt;
        b >> handle.i64 >> nItems >> fmt >> flags >> data;
        format = (OSPDataType)fmt;
      }

      NewTexture2d::NewTexture2d()
      {}
      
      NewTexture2d::NewTexture2d(ObjectHandle handle, vec2i dimensions,
                                 OSPTextureFormat format, void *texture, uint32 flags)
        : handle(handle),
          dimensions(dimensions),
          format(format),
          flags(flags)
      {
        size_t sz = ospray::sizeOf(format) * dimensions.x * dimensions.y;
        data.resize(sz);
        std::memcpy(data.data(), texture, sz);
      }
      
      void NewTexture2d::run()
      {
        Texture2D *texture = Texture2D::createTexture(dimensions, format, data.data(),
                                                      flags & ~OSP_TEXTURE_SHARED_BUFFER);
        Assert(texture);
        handle.assign(texture);
      }
      
      void NewTexture2d::serialize(WriteStream &b) const
      {
        b << (int64)handle << dimensions << (int32)format << flags << data;
      }
      
      void NewTexture2d::deserialize(ReadStream &b)
      {
        int32 fmt;
        b >> handle.i64 >> dimensions >> fmt >> flags >> data;
        format = (OSPTextureFormat)fmt;
      }

      SetRegion::SetRegion() {}
      SetRegion::SetRegion(OSPVolume volume, vec3i start, vec3i size,
                           const void *src, OSPDataType type)
        : handle((ObjectHandle&)volume), regionStart(start), regionSize(size), type(type)
      {
        size_t bytes = ospray::sizeOf(type) * size.x * size.y * size.z;
        // TODO: With the MPI batching this limitation should be lifted
        if (bytes > 2000000000LL) {
          throw std::runtime_error("MPI ospSetRegion does not support region sizes > 2GB");
        }
        data.resize(bytes);
        std::memcpy(data.data(), src, bytes);  //TODO: should support sending data without copy
      }
      void SetRegion::run() {
        Volume *volume = (Volume*)handle.lookup();
        Assert(volume);
        // TODO: Does it make sense to do the allreduce & report back fails?
        // TODO: Should we be allocating the data with alignedMalloc instead?
        // We could use a std::vector with an aligned std::allocator
        if (!volume->setRegion(data.data(), regionStart, regionSize)) {
          throw std::runtime_error("Failed to set region for volume");
        }
      }

      void SetRegion::serialize(WriteStream &b) const {
        b << (int64)handle << regionStart << regionSize << (int32)type << data;
      }
      void SetRegion::deserialize(ReadStream &b) {
        int32 ty;
        b >> handle.i64 >> regionStart >> regionSize >> ty >> data;
        type = (OSPDataType)ty;
      }

      ClearFrameBuffer::ClearFrameBuffer(){}
      ClearFrameBuffer::ClearFrameBuffer(OSPFrameBuffer fb, uint32 channels)
        : handle((ObjectHandle&)fb), channels(channels)
      {}
      void ClearFrameBuffer::run() {
        FrameBuffer *fb = (FrameBuffer*)handle.lookup();
        Assert(fb);
        fb->clear(channels);
      }
      void ClearFrameBuffer::runOnMaster() {
        run();
      }

      void ClearFrameBuffer::serialize(WriteStream &b) const {
        b << (int64)handle << channels;
      }
      void ClearFrameBuffer::deserialize(ReadStream &b) {
        b >> handle.i64 >> channels;
      }

      RenderFrame::RenderFrame() : varianceResult(0.f) {}
      RenderFrame::RenderFrame(OSPFrameBuffer fb, OSPRenderer renderer, uint32 channels)
        : fbHandle((ObjectHandle&)fb), rendererHandle((ObjectHandle&)renderer), channels(channels),
          varianceResult(0.f)
      {}
      void RenderFrame::run() {
        FrameBuffer *fb = (FrameBuffer*)fbHandle.lookup();
        Renderer *renderer = (Renderer*)rendererHandle.lookup();
        Assert(renderer);
        Assert(fb);
        // TODO: This function execution must run differently
        // if we're the master vs. the worker in master/worker
        // mode. Actually, if the Master has the Master load balancer,
        // it should be fine??? Actually not if the renderer
        // takes over scheduling of tile work like the distributed volume renderer
        // We need some way to pick the right function to call, either to the
        // renderer or directly to the load balancer to render the frame
#if 1
        varianceResult = renderer->renderFrame(fb, channels);
#else
        if (mpi::world.rank > 0) {
          renderer->renderFrame(fb, channels);
        } else {
          TiledLoadBalancer::instance->renderFrame(nullptr, fb, channels);
        }
#endif
      }
      void RenderFrame::runOnMaster() {
        Renderer *renderer = (Renderer*)rendererHandle.lookup();
        FrameBuffer *fb = (FrameBuffer*)fbHandle.lookup();
        Assert(renderer);
        Assert(fb);
        varianceResult = TiledLoadBalancer::instance->renderFrame(renderer, fb, channels);
      }
      void RenderFrame::serialize(WriteStream &b) const {
        b << (int64)fbHandle << (int64)rendererHandle << channels;
      }
      void RenderFrame::deserialize(ReadStream &b) {
        b >> fbHandle.i64 >> rendererHandle.i64 >> channels;
      }

      void AddGeometry::run()
      {
        Model *model = (Model*)modelHandle.lookup();
        Geometry *geometry = (Geometry*)objectHandle.lookup();
        Assert(model);
        Assert(geometry);
        model->geometry.push_back(geometry);
      }

      void AddVolume::run()
      {
        Model *model = (Model*)modelHandle.lookup();
        Volume *volume = (Volume*)objectHandle.lookup();
        Assert(model);
        Assert(volume);
        model->volume.push_back(volume);
      }

      void RemoveGeometry::run()
      {
        Model *model = (Model*)modelHandle.lookup();
        Geometry *geometry = (Geometry*)objectHandle.lookup();
        Assert(model);
        Assert(geometry);
        auto it = std::find_if(model->geometry.begin(), model->geometry.end(),
                               [&](const Ref<Geometry> &g) {
                                 return geometry == &*g;
                               });
        if (it != model->geometry.end()) {
          model->geometry.erase(it);
        }
      }

      void RemoveVolume::run() {
        Model *model = (Model*)modelHandle.lookup();
        Volume *volume = (Volume*)objectHandle.lookup();
        Assert(model);
        Assert(volume);
        model->volume.push_back(volume);
        auto it = std::find_if(model->volume.begin(), model->volume.end(),
                               [&](const Ref<Volume> &v) {
                                 return volume == &*v;
                               });
        if (it != model->volume.end()) {
          model->volume.erase(it);
        }
      }

      RemoveParam::RemoveParam()
      {}
      
      RemoveParam::RemoveParam(ObjectHandle handle, const char *name) : handle(handle), name(name) {
        Assert(handle != nullHandle);
      }
      
      void RemoveParam::run()
      {
        ManagedObject *obj = handle.lookup();
        Assert(obj);
        obj->removeParam(name.c_str());
      }
      
      void RemoveParam::runOnMaster()
      {
        ManagedObject *obj = handle.lookup();
        if (dynamic_cast<Renderer*>(obj) || dynamic_cast<Volume*>(obj)) {
          obj->removeParam(name.c_str());
        }
      }

      void RemoveParam::serialize(WriteStream &b) const
      {
        b << (int64)handle << name;
      }
      void RemoveParam::deserialize(ReadStream &b)
      {
        b >> handle.i64 >> name;
      }

      SetPixelOp::SetPixelOp(){}
      SetPixelOp::SetPixelOp(OSPFrameBuffer fb, OSPPixelOp op)
        : fbHandle((ObjectHandle&)fb), poHandle((ObjectHandle&)op)
      {}
      void SetPixelOp::run() {
        FrameBuffer *fb = (FrameBuffer*)fbHandle.lookup();
        PixelOp *po = (PixelOp*)poHandle.lookup();
        Assert(fb);
        Assert(po);
        fb->pixelOp = po->createInstance(fb, fb->pixelOp.ptr);
        if (!fb->pixelOp) {
          std::cout << "#osp:mpi: WARNING: PixelOp did not create an instance!" << std::endl;
        }
      }

      void SetPixelOp::serialize(WriteStream &b) const {
        b << (int64)fbHandle << (int64)poHandle;
      }
      void SetPixelOp::deserialize(ReadStream &b) {
        b >> fbHandle.i64 >> poHandle.i64;
      }

      CommandRelease::CommandRelease() {}
      CommandRelease::CommandRelease(ObjectHandle handle) : handle(handle){}
      void CommandRelease::run() {
        ManagedObject *obj = handle.lookup();
        Assert(obj);
        handle.freeObject();
      }

      void CommandRelease::serialize(WriteStream &b) const {
        b << (int64)handle;
      }
      void CommandRelease::deserialize(ReadStream &b) {
        b >> handle.i64;
      }

      CommandFinalize::CommandFinalize(){}
      void CommandFinalize::run() {
        async::shutdown();
        // TODO: Is it ok to call exit again here?
        // should we be calling exit? When the MPIDevice is
        // destroyed (at program exit) we'll send this command
        // to ourselves/other ranks. In master/worker mode
        // the workers should call std::exit to leave the worker loop
        // but the master or all ranks in collab mode would already
        // be exiting.
        std::exit(0);
      }
      void CommandFinalize::runOnMaster() {
        async::shutdown();
      }
      void CommandFinalize::serialize(WriteStream &b) const {}
      void CommandFinalize::deserialize(ReadStream &b) {}


    } // ::ospray::mpi::work
  } // ::ospray::mpi
} // ::ospray

