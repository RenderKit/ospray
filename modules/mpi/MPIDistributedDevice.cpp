// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#undef NDEBUG // do all assertions in this file

//ospray
#include "ospray/camera/Camera.h"
#include "ospray/common/Data.h"
#include "ospray/lights/Light.h"
#include "ospray/transferFunction/TransferFunction.h"
//mpiCommon
#include "mpiCommon/MPICommon.h"
//ospray_mpi
#include "mpi/MPIDistributedDevice.h"
#include "mpi/fb/DistributedFrameBuffer.h"
#include "mpi/render/MPILoadBalancer.h"

//distributed objects
#include "render/distributed/DistributedRaycast.h"
#include "common/DistributedModel.h"

#ifdef OPEN_MPI
# include <thread>
//# define _GNU_SOURCE
# include <sched.h>
#endif

namespace ospray {
  namespace mpi {

    // Helper functions ///////////////////////////////////////////////////////

    template <typename OSPRAY_TYPE, typename API_TYPE>
    inline API_TYPE createLocalObject(const char *type)
    {
      auto *instance = OSPRAY_TYPE::createInstance(type);
      instance->refInc();
      return (API_TYPE)instance;
    }

    template <typename OSPRAY_TYPE, typename API_TYPE>
    inline API_TYPE createDistributedObject(const char *type)
    {
      auto *instance = OSPRAY_TYPE::createInstance(type);
      instance->refInc();

      ObjectHandle handle;
      handle.assign(instance);

      return (API_TYPE)(int64)handle;
    }

    template <typename OSPRAY_TYPE>
    inline OSPRAY_TYPE& lookupDistributedObject(OSPObject obj)
    {
      auto &handle = reinterpret_cast<ObjectHandle&>(obj);
      auto *object = (OSPRAY_TYPE*)handle.lookup();

      if (!object)
        throw std::runtime_error("#dmpi: ObjectHandle doesn't exist!");

      return *object;
    }

    template <typename OSPRAY_TYPE>
    inline OSPRAY_TYPE* lookupObject(OSPObject obj)
    {
      auto &handle = reinterpret_cast<ObjectHandle&>(obj);
      if (handle.defined()) {
        return (OSPRAY_TYPE*)handle.lookup();
      } else {
        return (OSPRAY_TYPE*)obj;
      }
    }

    static void embreeErrorFunc(void *, const RTCError code, const char* str)
    {
      postStatusMsg() << "#osp: embree internal error " << code << " : " << str;
      throw std::runtime_error("embree internal error '" +std::string(str)+"'");
    }

    // MPIDistributedDevice definitions ///////////////////////////////////////

    MPIDistributedDevice::~MPIDistributedDevice()
    {
      if (shouldFinalizeMPI) {
        try {
          MPI_CALL(Finalize());
        } catch (...) {
          //TODO: anything to do here? try-catch added to silence a warning...
        }
      }
    }

    void MPIDistributedDevice::commit()
    {
      if (!initialized) {
        int _ac = 1;
        const char *_av[] = {"ospray_mpi_distributed_device"};

        shouldFinalizeMPI = mpicommon::init(&_ac, _av);

        embreeDevice = rtcNewDevice(generateEmbreeDeviceCfg(*this).c_str());

        rtcDeviceSetErrorFunction2(embreeDevice, embreeErrorFunc, nullptr);

        RTCError erc = rtcDeviceGetError(embreeDevice);
        if (erc != RTC_NO_ERROR) {
          // why did the error function not get called !?
          postStatusMsg() << "#osp:init: embree internal error number " << erc;
          assert(erc == RTC_NO_ERROR);
        }

        initialized = true;
      }

      Device::commit();

      masterRank = getParam1i("masterRank", 0);

      std::string mode = getParamString("mode", "distributed");

      if (mode == "distributed") {
        postStatusMsg() << "#dmpi: device commit() setting mode to " << mode;
      } else {
        throw std::runtime_error("#dmpi: bad device mode ['" + mode + "]");
      }

      TiledLoadBalancer::instance =
                      make_unique<staticLoadBalancer::Distributed>();
    }

    OSPFrameBuffer
    MPIDistributedDevice::frameBufferCreate(const vec2i &size,
                                            const OSPFrameBufferFormat mode,
                                            const uint32 channels)
    {
      const bool hasDepthBuffer    = channels & OSP_FB_DEPTH;
      const bool hasAccumBuffer    = channels & OSP_FB_ACCUM;
      const bool hasVarianceBuffer = channels & OSP_FB_VARIANCE;

      ObjectHandle handle;

      auto *instance = new DistributedFrameBuffer(size, handle, mode,
                                                  hasDepthBuffer,
                                                  hasAccumBuffer,
                                                  hasVarianceBuffer,
                                                  true);
      instance->refInc();

      handle.assign(instance);

      return (OSPFrameBuffer)(int64)handle;
    }


    const void*
    MPIDistributedDevice::frameBufferMap(OSPFrameBuffer _fb,
                                         OSPFrameBufferChannel channel)
    {
      if (!mpicommon::IamTheMaster())
        throw std::runtime_error("Can only map framebuffer on the master!");

      auto &fb = lookupDistributedObject<FrameBuffer>(_fb);

      switch (channel) {
      case OSP_FB_COLOR: return fb.mapColorBuffer();
      case OSP_FB_DEPTH: return fb.mapDepthBuffer();
      default: return nullptr;
      }
    }

    void MPIDistributedDevice::frameBufferUnmap(const void *mapped,
                                                OSPFrameBuffer _fb)
    {
      auto &fb = lookupDistributedObject<FrameBuffer>(_fb);
      fb.unmap(mapped);
    }

    void MPIDistributedDevice::frameBufferClear(OSPFrameBuffer _fb,
                                                const uint32 fbChannelFlags)
    {
      auto &fb = lookupDistributedObject<FrameBuffer>(_fb);
      fb.clear(fbChannelFlags);
    }

    OSPModel MPIDistributedDevice::newModel()
    {
      auto *instance = new DistributedModel;
      instance->refInc();

      ObjectHandle handle;
      handle.assign(instance);

      return (OSPModel)(int64)handle;
    }

    void MPIDistributedDevice::commit(OSPObject _object)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->commit();
    }

    void MPIDistributedDevice::addGeometry(OSPModel _model,
                                           OSPGeometry _geometry)
    {
      auto &model = lookupDistributedObject<Model>(_model);
      auto *geom  = lookupObject<Geometry>(_geometry);

      model.geometry.push_back(geom);
    }

    void MPIDistributedDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      auto &model  = lookupDistributedObject<Model>(_model);
      auto *volume = lookupObject<Volume>(_volume);

      model.volume.push_back(volume);
    }

    OSPData MPIDistributedDevice::newData(size_t nitems, OSPDataType format,
                                          const void *init, int flags)
    {
      auto *instance = new Data(nitems, format, init, flags);
      instance->refInc();
      return (OSPData)instance;
    }

    void MPIDistributedDevice::setVoidPtr(OSPObject _object,
                                          const char *bufName,
                                          void *v)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::removeParam(OSPObject _object, const char *name)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->removeParam(name);
    }

    int MPIDistributedDevice::setRegion(OSPVolume _volume, const void *source,
                                        const vec3i &index, const vec3i &count)
    {
      auto *volume = (Volume*)_volume;
      return volume->setRegion(source, index, count);
    }

    void MPIDistributedDevice::setString(OSPObject _object,
                                         const char *bufName,
                                         const char *s)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->findParam(bufName, true)->set(s);
    }

    int MPIDistributedDevice::loadModule(const char *name)
    {
      return loadLocalModule(name);
    }

    void MPIDistributedDevice::setFloat(OSPObject _object,
                                        const char *bufName,
                                        const float f)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->findParam(bufName, true)->set(f);
    }

    void MPIDistributedDevice::setInt(OSPObject _object,
                                      const char *bufName,
                                      const int i)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->findParam(bufName, true)->set(i);
    }

    void MPIDistributedDevice::setVec2f(OSPObject _object,
                                        const char *bufName,
                                        const vec2f &v)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setVec3f(OSPObject _object,
                                        const char *bufName,
                                        const vec3f &v)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setVec4f(OSPObject _object,
                                        const char *bufName,
                                        const vec4f &v)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setVec2i(OSPObject _object,
                                        const char *bufName,
                                        const vec2i &v)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setVec3i(OSPObject _object,
                                        const char *bufName,
                                        const vec3i &v)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      object->findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setObject(OSPObject _object,
                                         const char *bufName,
                                         OSPObject _value)
    {
      auto *object = lookupObject<ManagedObject>(_object);
      auto *value  = lookupObject<ManagedObject>(_value);
      object->set(bufName, value);
    }

    OSPPixelOp MPIDistributedDevice::newPixelOp(const char *type)
    {
      return createDistributedObject<PixelOp, OSPPixelOp>(type);
    }

    void MPIDistributedDevice::setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op)
    {
      auto &fb = lookupDistributedObject<FrameBuffer>(_fb);
      auto &op = lookupDistributedObject<PixelOp>(_op);
      fb.pixelOp = op.createInstance(&fb, fb.pixelOp.ptr);
    }

    OSPRenderer MPIDistributedDevice::newRenderer(const char *type)
    {
      return createDistributedObject<Renderer, OSPRenderer>(type);
    }

    OSPCamera MPIDistributedDevice::newCamera(const char *type)
    {
      return createLocalObject<Camera, OSPCamera>(type);
    }

    OSPVolume MPIDistributedDevice::newVolume(const char *type)
    {
      return createLocalObject<Volume, OSPVolume>(type);
    }

    OSPGeometry MPIDistributedDevice::newGeometry(const char *type)
    {
      return createLocalObject<Geometry, OSPGeometry>(type);
    }

    OSPMaterial MPIDistributedDevice::newMaterial(OSPRenderer, const char *)
    {
      NOT_IMPLEMENTED;
    }

    OSPTransferFunction
    MPIDistributedDevice::newTransferFunction(const char *type)
    {
      return createLocalObject<TransferFunction,
                               OSPTransferFunction>(type);
    }

    OSPLight MPIDistributedDevice::newLight(OSPRenderer _renderer,
                                            const char *type)
    {
      auto &renderer = lookupDistributedObject<Renderer>(_renderer);
      auto *light    = renderer.createLight(type);

      if (light == nullptr)
        light = Light::createLight(type);

      if (light) {
        return (OSPLight)light;
      } else {
        return nullptr;
      }
    }

    void MPIDistributedDevice::removeGeometry(OSPModel _model,
                                              OSPGeometry _geometry)
    {
      auto &model = lookupDistributedObject<Model>(_model);
      auto *geom  = lookupObject<Geometry>(_geometry);

      //TODO: confirm this works?
      model.geometry.erase(std::remove(model.geometry.begin(),
                                       model.geometry.end(),
                                       Ref<Geometry>(geom)));
    }

    void MPIDistributedDevice::removeVolume(OSPModel _model, OSPVolume _volume)
    {
      auto &model  = lookupDistributedObject<Model>(_model);
      auto *volume = lookupObject<Volume>(_volume);

      //TODO: confirm this works?
      model.volume.erase(std::remove(model.volume.begin(),
                                     model.volume.end(),
                                     Ref<Volume>(volume)));
    }

    float MPIDistributedDevice::renderFrame(OSPFrameBuffer _fb,
                                            OSPRenderer _renderer,
                                            const uint32 fbChannelFlags)
    {
      auto &fb       = lookupDistributedObject<FrameBuffer>(_fb);
      auto &renderer = lookupDistributedObject<Renderer>(_renderer);
      auto result    = renderer.renderFrame(&fb, fbChannelFlags);
      mpicommon::world.barrier();
      return result;
    }

    void MPIDistributedDevice::release(OSPObject _obj)
    {
      if (!_obj) return;
      auto *obj = lookupObject<ManagedObject>(_obj);
      obj->refDec();
    }

    void MPIDistributedDevice::setMaterial(OSPGeometry, OSPMaterial)
    {
      NOT_IMPLEMENTED;
    }

    OSPTexture2D MPIDistributedDevice::newTexture2D(
      const vec2i &,
      const OSPTextureFormat,
      void *,
      const uint32
    )
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::sampleVolume(float **,
                                            OSPVolume,
                                            const vec3f *,
                                            const size_t &)
    {
      NOT_IMPLEMENTED;
    }

    OSP_REGISTER_DEVICE(MPIDistributedDevice, mpi_distributed_device);
    OSP_REGISTER_DEVICE(MPIDistributedDevice, mpi_distributed);

  } // ::ospray::mpi
} // ::ospray
