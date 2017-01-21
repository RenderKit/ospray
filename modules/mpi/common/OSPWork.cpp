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

namespace ospray {
  namespace mpi {
    namespace work {

#define REGISTER_WORK_UNIT(W) W::TAG, make_work_unit<W>

      void initWorkMap()
      {
        Work::WORK_MAP = Work::WorkMap{
          { REGISTER_WORK_UNIT(NewObject<Renderer>) },
          { REGISTER_WORK_UNIT(NewObject<Model>) },
          { REGISTER_WORK_UNIT(NewObject<Geometry>) },
          { REGISTER_WORK_UNIT(NewObject<Camera>) },
          { REGISTER_WORK_UNIT(NewObject<Volume>) },
          { REGISTER_WORK_UNIT(NewObject<TransferFunction>) },
          { REGISTER_WORK_UNIT(NewObject<PixelOp>) },

          { REGISTER_WORK_UNIT(NewRendererObject<Material>) },
          { REGISTER_WORK_UNIT(NewRendererObject<Light>) },

          { REGISTER_WORK_UNIT(NewData) },
          { REGISTER_WORK_UNIT(NewTexture2d) },

          { REGISTER_WORK_UNIT(CommitObject) },
          { REGISTER_WORK_UNIT(CommandRelease) },

          { REGISTER_WORK_UNIT(LoadModule) },

          { REGISTER_WORK_UNIT(AddObject<OSPGeometry>) },
          { REGISTER_WORK_UNIT(AddObject<OSPVolume>) },
          { REGISTER_WORK_UNIT(RemoveObject<OSPGeometry>) },
          { REGISTER_WORK_UNIT(RemoveObject<OSPVolume>) },

          { REGISTER_WORK_UNIT(CreateFrameBuffer) },
          { REGISTER_WORK_UNIT(ClearFrameBuffer) },
          { REGISTER_WORK_UNIT(RenderFrame) },

          { REGISTER_WORK_UNIT(SetRegion) },
          { REGISTER_WORK_UNIT(SetPixelOp) },

          { REGISTER_WORK_UNIT(SetParam<OSPMaterial>) },
          { REGISTER_WORK_UNIT(SetParam<OSPObject>) },
          { REGISTER_WORK_UNIT(SetParam<std::string>) },
          { REGISTER_WORK_UNIT(SetParam<int>) },
          { REGISTER_WORK_UNIT(SetParam<float>) },
          { REGISTER_WORK_UNIT(SetParam<vec2f>) },
          { REGISTER_WORK_UNIT(SetParam<vec2i>) },
          { REGISTER_WORK_UNIT(SetParam<vec3f>) },
          { REGISTER_WORK_UNIT(SetParam<vec3i>) },
          { REGISTER_WORK_UNIT(SetParam<vec4f>) },

          { REGISTER_WORK_UNIT(RemoveParam) },

          { REGISTER_WORK_UNIT(CommandFinalize) }
        };
      }

#undef REGISTER_WORK_UNIT

      // All the tags so they can be linked in properly
      const size_t NewObjectTag<Renderer>::TAG;
      const size_t NewObjectTag<Model>::TAG;
      const size_t NewObjectTag<Geometry>::TAG;
      const size_t NewObjectTag<Camera>::TAG;
      const size_t NewObjectTag<Volume>::TAG;
      const size_t NewObjectTag<TransferFunction>::TAG;
      const size_t NewObjectTag<PixelOp>::TAG;
      template<typename T>
      const size_t NewObject<T>::TAG;
      // should they init here or in the header? Header probably?
      const size_t NewRendererObjectTag<Material>::TAG;
      const size_t NewRendererObjectTag<Light>::TAG;
      template<typename T>
      const size_t NewRendererObject<T>::TAG;
      const size_t NewData::TAG;
      const size_t NewTexture2d::TAG;
      const size_t SetRegion::TAG;
      const size_t CommitObject::TAG;
      const size_t ClearFrameBuffer::TAG;
      const size_t RenderFrame::TAG;
      const size_t AddObjectTag<OSPGeometry>::TAG;
      const size_t AddObjectTag<OSPVolume>::TAG;
      template<typename T>
      const size_t AddObject<T>::TAG;
      const size_t RemoveObjectTag<OSPGeometry>::TAG;
      const size_t RemoveObjectTag<OSPVolume>::TAG;
      template<typename T>
      const size_t RemoveObject<T>::TAG;
      const size_t CreateFrameBuffer::TAG;
      const size_t ParamTag<std::string>::TAG;
      const size_t ParamTag<int>::TAG;
      const size_t ParamTag<float>::TAG;
      const size_t ParamTag<vec2f>::TAG;
      const size_t ParamTag<vec2i>::TAG;
      const size_t ParamTag<vec3f>::TAG;
      const size_t ParamTag<vec3i>::TAG;
      const size_t ParamTag<vec4f>::TAG;
      template<typename T>
      const size_t SetParam<T>::TAG;
      const size_t SetParam<OSPMaterial>::TAG;
      const size_t SetParam<OSPObject>::TAG;
      const size_t RemoveParam::TAG;
      const size_t SetPixelOp::TAG;
      const size_t CommandRelease::TAG;
      const size_t LoadModule::TAG;
      const size_t CommandFinalize::TAG;

      template<>
      void NewObject<Renderer>::run() {
        Renderer *renderer = Renderer::createRenderer(type.c_str());
        if (!renderer) {
          throw std::runtime_error("unknown renderer type '" + type + "'");
        }
        handle.assign(renderer);
      }
      template<>
      void NewObject<Renderer>::runOnMaster() {
        run();
      }
      template<>
      void NewObject<Model>::run() {
        Model *model = new Model;
        handle.assign(model);
      }
      template<>
      void NewObject<Geometry>::run() {
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
      void NewObject<Camera>::run() {
        Camera *camera = Camera::createCamera(type.c_str());
        Assert(camera);
        handle.assign(camera);
      }
      template<>
      void NewObject<Volume>::run() {
        Volume *volume = Volume::createInstance(type.c_str());
        if (!volume) {
          throw std::runtime_error("unknown volume type '" + type + "'");
        }
        volume->refInc();
        handle.assign(volume);
      }
      template<>
      void NewObject<Volume>::runOnMaster() {
        run();
      }
      template<>
      void NewObject<TransferFunction>::run() {
        TransferFunction *tfn = TransferFunction::createInstance(type.c_str());
        if (!tfn) {
          throw std::runtime_error("unknown transfer functon type '" + type + "'");
        }
        tfn->refInc();
        handle.assign(tfn);
      }
      template<>
      void NewObject<PixelOp>::run() {
        PixelOp *pixelOp = PixelOp::createPixelOp(type.c_str());
        if (!pixelOp) {
          throw std::runtime_error("unknown pixel op type '" + type + "'");
        }
        handle.assign(pixelOp);
      }

      template<>
      void NewRendererObject<Material>::run() {
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
      template<>
      void NewRendererObject<Light>::run() {
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

      NewData::NewData(){}
      NewData::NewData(ObjectHandle handle, size_t nItems,
          OSPDataType format, void *init, int flags)
        : handle(handle), nItems(nItems), format(format), localData(nullptr), flags(flags)
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
      void NewData::run() {
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
      size_t NewData::getTag() const {
        return TAG;
      }
      void NewData::serialize(SerialBuffer &b) const {
        b << (int64)handle << nItems << (int32)format << flags << data;
      }
      void NewData::deserialize(SerialBuffer &b) {
        int32 fmt;
        b >> handle.i64 >> nItems >> fmt >> flags >> data;
        format = (OSPDataType)fmt;
      }

      NewTexture2d::NewTexture2d() {}
      NewTexture2d::NewTexture2d(ObjectHandle handle, vec2i dimensions,
          OSPTextureFormat format, void *texture, uint32 flags)
        : handle(handle), dimensions(dimensions), format(format), flags(flags)
      {
        size_t sz = ospray::sizeOf(format) * dimensions.x * dimensions.y;
        data.resize(sz);
        std::memcpy(data.data(), texture, sz);
      }
      void NewTexture2d::run() {
        Texture2D *texture = Texture2D::createTexture(dimensions, format, data.data(),
                                                      flags & ~OSP_TEXTURE_SHARED_BUFFER);
        Assert(texture);
        handle.assign(texture);
      }
      size_t NewTexture2d::getTag() const {
        return TAG;
      }
      void NewTexture2d::serialize(SerialBuffer &b) const {
        b << (int64)handle << dimensions << (int32)format << flags << data;
      }
      void NewTexture2d::deserialize(SerialBuffer &b) {
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
      size_t SetRegion::getTag() const {
        return TAG;
      }
      void SetRegion::serialize(SerialBuffer &b) const {
        b << (int64)handle << regionStart << regionSize << (int32)type << data;
      }
      void SetRegion::deserialize(SerialBuffer &b) {
        int32 ty;
        b >> handle.i64 >> regionStart >> regionSize >> ty >> data;
        type = (OSPDataType)ty;
      }

      CommitObject::CommitObject(){}
      CommitObject::CommitObject(ObjectHandle handle) : handle(handle) {}
      void CommitObject::run() {
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
        mpi::barrier(mpi::world);
      }
      void CommitObject::runOnMaster() {
        ManagedObject *obj = handle.lookup();
        if (dynamic_cast<Renderer*>(obj)) {
          obj->commit();
        }
        mpi::barrier(mpi::world);
      }
      size_t CommitObject::getTag() const {
        return TAG;
      }
      bool CommitObject::flushing() const {
        return true;
      }
      void CommitObject::serialize(SerialBuffer &b) const {
        b << (int64)handle;
      }
      void CommitObject::deserialize(SerialBuffer &b) {
        b >> handle.i64;
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
      size_t ClearFrameBuffer::getTag() const {
        return TAG;
      }
      void ClearFrameBuffer::serialize(SerialBuffer &b) const {
        b << (int64)handle << channels;
      }
      void ClearFrameBuffer::deserialize(SerialBuffer &b) {
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
      size_t RenderFrame::getTag() const {
        return TAG;
      }
      bool RenderFrame::flushing() const {
        return true;
      }
      void RenderFrame::serialize(SerialBuffer &b) const {
        b << (int64)fbHandle << (int64)rendererHandle << channels;
      }
      void RenderFrame::deserialize(SerialBuffer &b) {
        b >> fbHandle.i64 >> rendererHandle.i64 >> channels;
      }

      template<>
      void AddObject<OSPGeometry>::run() {
        Model *model = (Model*)modelHandle.lookup();
        Geometry *geometry = (Geometry*)objectHandle.lookup();
        Assert(model);
        Assert(geometry);
        model->geometry.push_back(geometry);
      }
      template<>
      void AddObject<OSPVolume>::run() {
        Model *model = (Model*)modelHandle.lookup();
        Volume *volume = (Volume*)objectHandle.lookup();
        Assert(model);
        Assert(volume);
        model->volume.push_back(volume);
      }

      template<>
      void RemoveObject<OSPGeometry>::run() {
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
      template<>
      void RemoveObject<OSPVolume>::run() {
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

      CreateFrameBuffer::CreateFrameBuffer() {}
      CreateFrameBuffer::CreateFrameBuffer(ObjectHandle handle, vec2i dimensions,
          OSPFrameBufferFormat format, uint32 channels)
        : handle(handle), dimensions(dimensions), format(format), channels(channels)
      {}
      void CreateFrameBuffer::run() {
        const bool hasDepthBuffer = channels & OSP_FB_DEPTH;
        const bool hasAccumBuffer = channels & OSP_FB_ACCUM;
        const bool hasVarianceBuffer = channels & OSP_FB_VARIANCE;
        FrameBuffer *fb = new DistributedFrameBuffer(ospray::mpi::async::CommLayer::WORLD,
            dimensions, handle, format, hasDepthBuffer, hasAccumBuffer, hasVarianceBuffer);

        // TODO: Only the master does this increment, though should the workers do it too?
        fb->refInc();
        handle.assign(fb);
      }
      void CreateFrameBuffer::runOnMaster() {
        run();
      }
      size_t CreateFrameBuffer::getTag() const {
        return TAG;
      }
      void CreateFrameBuffer::serialize(SerialBuffer &b) const {
        b << (int64)handle << dimensions << (int32)format << channels;
      }
      void CreateFrameBuffer::deserialize(SerialBuffer &b) {
        int32 fmt;
        b >> handle.i64 >> dimensions >> fmt >> channels;
        format = (OSPFrameBufferFormat)fmt;
      }

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

      RemoveParam::RemoveParam(){}
      RemoveParam::RemoveParam(ObjectHandle handle, const char *name) : handle(handle), name(name) {
        Assert(handle != nullHandle);
      }
      void RemoveParam::run() {
        ManagedObject *obj = handle.lookup();
        Assert(obj);
        obj->removeParam(name.c_str());
      }
      void RemoveParam::runOnMaster() {
        ManagedObject *obj = handle.lookup();
        if (dynamic_cast<Renderer*>(obj) || dynamic_cast<Volume*>(obj)) {
          obj->removeParam(name.c_str());
        }
      }
      size_t RemoveParam::getTag() const {
        return TAG;
      }
      void RemoveParam::serialize(SerialBuffer &b) const {
        b << (int64)handle << name;
      }
      void RemoveParam::deserialize(SerialBuffer &b) {
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
      size_t SetPixelOp::getTag() const {
        return TAG;
      }
      void SetPixelOp::serialize(SerialBuffer &b) const {
        b << (int64)fbHandle << (int64)poHandle;
      }
      void SetPixelOp::deserialize(SerialBuffer &b) {
        b >> fbHandle.i64 >> poHandle.i64;
      }

      CommandRelease::CommandRelease() {}
      CommandRelease::CommandRelease(ObjectHandle handle) : handle(handle){}
      void CommandRelease::run() {
        ManagedObject *obj = handle.lookup();
        Assert(obj);
        handle.freeObject();
      }
      size_t CommandRelease::getTag() const {
        return TAG;
      }
      void CommandRelease::serialize(SerialBuffer &b) const {
        b << (int64)handle;
      }
      void CommandRelease::deserialize(SerialBuffer &b) {
        b >> handle.i64;
      }

      LoadModule::LoadModule(){}
      LoadModule::LoadModule(const std::string &name) : name(name){}
      void LoadModule::run() {
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
      size_t LoadModule::getTag() const {
        return TAG;
      }
      bool LoadModule::flushing() const {
        return true;
      }
      void LoadModule::serialize(SerialBuffer &b) const {
        b << name;
      }
      void LoadModule::deserialize(SerialBuffer &b) {
        b >> name;
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
      size_t CommandFinalize::getTag() const {
        return TAG;
      }
      bool CommandFinalize::flushing() const {
        return true;
      }
      void CommandFinalize::serialize(SerialBuffer &b) const {}
      void CommandFinalize::deserialize(SerialBuffer &b) {}
    }
  }
}

