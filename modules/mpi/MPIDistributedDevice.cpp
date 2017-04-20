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
//mpiCommon
#include "mpiCommon/MPICommon.h"
//ospray_mpi
#include "mpi/MPIDistributedDevice.h"
#include "mpi/fb/DistributedFrameBuffer.h"
#include "mpi/render/MPILoadBalancer.h"

//distributed objects
#include "render/distributed/DistributedRaycast.h"

#ifdef OPEN_MPI
# include <thread>
//# define _GNU_SOURCE
# include <sched.h>
#endif

namespace ospray {
  namespace mpi {

    // Helper functions ///////////////////////////////////////////////////////

    template <typename OSPRAY_TYPE, typename API_TYPE>
    inline API_TYPE createOSPRayObjectWithHandle(const char *type)
    {
      auto *instance = OSPRAY_TYPE::createInstance(type);
      instance->refInc();

      ObjectHandle handle;
      handle.assign(instance);

      return (API_TYPE)(int64)handle;
    }

    template <typename OSPRAY_TYPE>
    inline OSPRAY_TYPE& objectFromAPIHandle(OSPObject obj)
    {
      auto &handle = reinterpret_cast<ObjectHandle&>(obj);
      auto *object = (OSPRAY_TYPE*)handle.lookup();

      if (!object)
        throw std::runtime_error("#dmpi: ObjectHandle doesn't exist!");

      return *object;
    }

    static void embreeErrorFunc(const RTCError code, const char* str)
    {
      postErrorMsg() << "#osp: embree internal error " << code << " : " << str;
      throw std::runtime_error("embree internal error '" +std::string(str)+"'");
    }

    // MPIDistributedDevice definitions ///////////////////////////////////////

    MPIDistributedDevice::~MPIDistributedDevice()
    {
      MPI_CALL(Finalize());
    }

    void MPIDistributedDevice::commit()
    {
      //TODO: is it necessary to track if we've initialized the device yet?
#if 0
      if (initialized)
        return;
#endif

      Device::commit();

      initialized = true;

      int _ac = 1;
      const char *_av[] = {"ospray_mpi_worker"};

      mpi::init(&_ac, _av);

      masterRank = getParam1i("masterRank", 0);

      std::string mode = getParamString("mode", "distributed");

      if (mode == "distributed") {
        postErrorMsg() << "#dmpi: device commit() setting mode to " << mode;
      } else {
        throw std::runtime_error("#dmpi: bad device mode ['" + mode + "]");
      }

      std::stringstream embreeConfig;
      if (debugMode)
        embreeConfig << " threads=1,verbose=2";
      else if(numThreads > 0)
        embreeConfig << " threads=" << numThreads;
      embreeDevice = rtcNewDevice(embreeConfig.str().c_str());

      rtcDeviceSetErrorFunction(embreeDevice, embreeErrorFunc);

      RTCError erc = rtcDeviceGetError(embreeDevice);
      if (erc != RTC_NO_ERROR) {
        // why did the error function not get called !?
        postErrorMsg() << "#osp:init: embree internal error number " << erc;
        assert(erc == RTC_NO_ERROR);
      }

      // TODO: implement 'staticLoadBalancer::Distributed'
      //TiledLoadBalancer::instance = make_unique<staticLoadBalancer::Master>();
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
                                                  hasVarianceBuffer);
      instance->refInc();

      handle.assign(instance);

      return (OSPFrameBuffer)(int64)handle;
    }


    const void*
    MPIDistributedDevice::frameBufferMap(OSPFrameBuffer _fb,
                                         OSPFrameBufferChannel channel)
    {
      if (!IamTheMaster())
        throw std::runtime_error("Can only map framebuffer on the master!");

      auto &fb = objectFromAPIHandle<FrameBuffer>(_fb);

      switch (channel) {
      case OSP_FB_COLOR: return fb.mapColorBuffer();
      case OSP_FB_DEPTH: return fb.mapDepthBuffer();
      default: return nullptr;
      }
    }

    void MPIDistributedDevice::frameBufferUnmap(const void *mapped,
                                                OSPFrameBuffer _fb)
    {
      auto &fb = objectFromAPIHandle<FrameBuffer>(_fb);
      fb.unmap(mapped);
    }

    void MPIDistributedDevice::frameBufferClear(OSPFrameBuffer _fb,
                                                const uint32 fbChannelFlags)
    {
      auto &fb = objectFromAPIHandle<FrameBuffer>(_fb);
      fb.clear(fbChannelFlags);
    }

    OSPModel MPIDistributedDevice::newModel()
    {
      auto *instance = new Model;
      instance->refInc();

      ObjectHandle handle;
      handle.assign(instance);

      return (OSPModel)(int64)handle;
    }

    void MPIDistributedDevice::commit(OSPObject _object)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.commit();
    }

    void MPIDistributedDevice::addGeometry(OSPModel _model,
                                           OSPGeometry _geometry)
    {
      auto &model = objectFromAPIHandle<Model>(_model);
      auto &geom  = objectFromAPIHandle<Geometry>(_geometry);

      model.geometry.push_back(&geom);
    }

    void MPIDistributedDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      NOT_IMPLEMENTED;
    }

    OSPData MPIDistributedDevice::newData(size_t nitems, OSPDataType format,
                                          void *init, int flags)
    {
      ObjectHandle handle;

      auto *instance = new Data(nitems, format, init, flags);
      instance->refInc();

      handle.assign(instance);

      return (OSPData)(int64)handle;
    }

    void MPIDistributedDevice::setVoidPtr(OSPObject _object,
                                          const char *bufName,
                                          void *v)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::removeParam(OSPObject object, const char *name)
    {
      NOT_IMPLEMENTED;
    }

    int MPIDistributedDevice::setRegion(OSPVolume _volume, const void *source,
                                        const vec3i &index, const vec3i &count)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setString(OSPObject _object,
                                         const char *bufName,
                                         const char *s)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.findParam(bufName, true)->set(s);
    }

    int MPIDistributedDevice::loadModule(const char *name)
    {
      loadLocalModule(name);
    }

    void MPIDistributedDevice::setFloat(OSPObject _object,
                                        const char *bufName,
                                        const float f)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.findParam(bufName, true)->set(f);
    }

    void MPIDistributedDevice::setInt(OSPObject _object,
                                      const char *bufName,
                                      const int i)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.findParam(bufName, true)->set(i);
    }

    void MPIDistributedDevice::setVec2f(OSPObject _object,
                                        const char *bufName,
                                        const vec2f &v)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setVec3f(OSPObject _object,
                                        const char *bufName,
                                        const vec3f &v)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setVec4f(OSPObject _object,
                                        const char *bufName,
                                        const vec4f &v)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setVec2i(OSPObject _object,
                                        const char *bufName,
                                        const vec2i &v)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setVec3i(OSPObject _object,
                                        const char *bufName,
                                        const vec3i &v)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      object.findParam(bufName, true)->set(v);
    }

    void MPIDistributedDevice::setObject(OSPObject _object,
                                         const char *bufName,
                                         OSPObject _value)
    {
      auto &object = objectFromAPIHandle<ManagedObject>(_object);
      auto &value  = objectFromAPIHandle<ManagedObject>(_value);
      object.set(bufName, &value);
    }

    OSPPixelOp MPIDistributedDevice::newPixelOp(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op)
    {
      NOT_IMPLEMENTED;
    }

    OSPRenderer MPIDistributedDevice::newRenderer(const char *type)
    {
      UNUSED(type);
      auto *instance = new DistributedRaycastRenderer;

      ObjectHandle handle;
      handle.assign(instance);

      return (OSPRenderer)(int64)handle;
    }

    OSPCamera MPIDistributedDevice::newCamera(const char *type)
    {
      return createOSPRayObjectWithHandle<Camera, OSPCamera>(type);
    }

    OSPVolume MPIDistributedDevice::newVolume(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPGeometry MPIDistributedDevice::newGeometry(const char *type)
    {
      return createOSPRayObjectWithHandle<Geometry, OSPGeometry>(type);
    }

    OSPMaterial MPIDistributedDevice::newMaterial(OSPRenderer _renderer,
                                                  const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPTransferFunction
    MPIDistributedDevice::newTransferFunction(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPLight MPIDistributedDevice::newLight(OSPRenderer _renderer,
                                            const char *type)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::removeGeometry(OSPModel _model,
                                              OSPGeometry _geometry)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::removeVolume(OSPModel _model, OSPVolume _volume)
    {
      NOT_IMPLEMENTED;
    }

    float MPIDistributedDevice::renderFrame(OSPFrameBuffer _fb,
                                            OSPRenderer _renderer,
                                            const uint32 fbChannelFlags)
    {
      auto &fb       = objectFromAPIHandle<FrameBuffer>(_fb);
      auto &renderer = objectFromAPIHandle<Renderer>(_renderer);
      renderer.renderFrame(&fb, fbChannelFlags);
    }

    void MPIDistributedDevice::release(OSPObject _obj)
    {
      UNUSED(_obj);
      postErrorMsg(1) << "WARNING: release() not yet implemented, memory leak!";
    }

    void MPIDistributedDevice::setMaterial(OSPGeometry _geometry,
                                           OSPMaterial _material)
    {
      NOT_IMPLEMENTED;
    }

    OSPTexture2D MPIDistributedDevice::newTexture2D(
      const vec2i &sz,
      const OSPTextureFormat type,
      void *data,
      const uint32 flags
    )
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::sampleVolume(float **results,
                                            OSPVolume volume,
                                            const vec3f *worldCoordinates,
                                            const size_t &count)
    {
      NOT_IMPLEMENTED;
    }

    OSP_REGISTER_DEVICE(MPIDistributedDevice, mpi_distributed_device);
    OSP_REGISTER_DEVICE(MPIDistributedDevice, mpi_distributed);

  } // ::ospray::mpi
} // ::ospray
