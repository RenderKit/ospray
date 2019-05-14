// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

// ospray
#include "ISPCDevice.h"
#include "camera/Camera.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/Material.h"
#include "common/Util.h"
#include "common/World.h"
#include "fb/LocalFB.h"
#include "geometry/GeometryInstance.h"
#include "lights/Light.h"
#include "render/LoadBalancer.h"
#include "render/RenderTask.h"
#include "render/Renderer.h"
#include "texture/Texture.h"
#include "texture/Texture2D.h"
#include "transferFunction/TransferFunction.h"
#include "volume/VolumeInstance.h"
// stl
#include <algorithm>

extern "C" RTCDevice ispc_embreeDevice()
{
  return ospray::api::ISPCDevice::embreeDevice;
}

namespace ospray {
  namespace api {

    RTCDevice ISPCDevice::embreeDevice = nullptr;

    ISPCDevice::~ISPCDevice()
    {
      try {
        if (embreeDevice) {
          rtcReleaseDevice(embreeDevice);
          embreeDevice = nullptr;
        }
      } catch (...) {
        // silently move on, sometimes a pthread mutex lock fails in Embree
      }
    }

    static void embreeErrorFunc(void *, const RTCError code, const char *str)
    {
      postStatusMsg() << "#osp: embree internal error " << code << " : " << str;
      throw std::runtime_error("embree internal error '" + std::string(str) +
                               "'");
    }

    void ISPCDevice::commit()
    {
      Device::commit();

      if (!embreeDevice) {
        // -------------------------------------------------------
        // initialize embree. (we need to do this here rather than in
        // ospray::init() because in mpi-mode the latter is also called
        // in the host-stubs, where it shouldn't.
        // -------------------------------------------------------
        embreeDevice = rtcNewDevice(generateEmbreeDeviceCfg(*this).c_str());
        rtcSetDeviceErrorFunction(embreeDevice, embreeErrorFunc, nullptr);
        RTCError erc = rtcGetDeviceError(embreeDevice);
        if (erc != RTC_ERROR_NONE) {
          // why did the error function not get called !?
          postStatusMsg() << "#osp:init: embree internal error number " << erc;
          assert(erc == RTC_ERROR_NONE);
        }
      }

      TiledLoadBalancer::instance = make_unique<LocalTiledLoadBalancer>();
    }

    OSPFrameBuffer ISPCDevice::frameBufferCreate(
        const vec2i &size,
        const OSPFrameBufferFormat mode,
        const uint32 channels)
    {
      FrameBuffer::ColorBufferFormat colorBufferFormat = mode;

      FrameBuffer *fb = new LocalFrameBuffer(size, colorBufferFormat, channels);
      return (OSPFrameBuffer)fb;
    }

    void ISPCDevice::resetAccumulation(OSPFrameBuffer _fb)
    {
      LocalFrameBuffer *fb = (LocalFrameBuffer *)_fb;
      fb->clear();
    }

    const void *ISPCDevice::frameBufferMap(OSPFrameBuffer _fb,
                                           OSPFrameBufferChannel channel)
    {
      LocalFrameBuffer *fb = (LocalFrameBuffer *)_fb;
      return fb->mapBuffer(channel);
    }

    void ISPCDevice::frameBufferUnmap(const void *mapped, OSPFrameBuffer _fb)
    {
      FrameBuffer *fb = (FrameBuffer *)_fb;
      fb->unmap(mapped);
    }

    OSPWorld ISPCDevice::newWorld()
    {
      return (OSPWorld) new World;
    }

    void ISPCDevice::commit(OSPObject _object)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->commit();
    }

    OSPData ISPCDevice::newData(size_t nitems,
                                OSPDataType format,
                                const void *init,
                                int flags)
    {
      Data *data = new Data(nitems, format, init, flags);
      return (OSPData)data;
    }

    void ISPCDevice::setString(OSPObject _object,
                               const char *bufName,
                               const char *s)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam<std::string>(bufName, s);
    }

    void ISPCDevice::setVoidPtr(OSPObject _object, const char *bufName, void *v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::removeParam(OSPObject _object, const char *name)
    {
      ManagedObject *object = (ManagedObject *)_object;
      ManagedObject *existing =
          object->getParam<ManagedObject *>(name, nullptr);
      if (existing)
        existing->refDec();
      object->removeParam(name);
    }

    void ISPCDevice::setInt(OSPObject _object, const char *bufName, const int f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, f);
    }

    void ISPCDevice::setBool(OSPObject _object,
                             const char *bufName,
                             const bool b)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, b);
    }

    void ISPCDevice::setFloat(OSPObject _object,
                              const char *bufName,
                              const float f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, f);
    }

    int ISPCDevice::setRegion(OSPVolume handle,
                              const void *source,
                              const vec3i &index,
                              const vec3i &count)
    {
      Volume *volume = (Volume *)handle;
      return volume->setRegion(source, index, count);
    }

    void ISPCDevice::setVec2f(OSPObject _object,
                              const char *bufName,
                              const vec2f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setVec3f(OSPObject _object,
                              const char *bufName,
                              const vec3f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setVec4f(OSPObject _object,
                              const char *bufName,
                              const vec4f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setVec2i(OSPObject _object,
                              const char *bufName,
                              const vec2i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setVec3i(OSPObject _object,
                              const char *bufName,
                              const vec3i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setObject(OSPObject _target,
                               const char *bufName,
                               OSPObject _value)
    {
      ManagedObject *target = (ManagedObject *)_target;
      ManagedObject *value  = (ManagedObject *)_value;
      target->setParam(bufName, value);
    }

    OSPPixelOp ISPCDevice::newPixelOp(const char *type)
    {
      return (OSPPixelOp)PixelOp::createInstance(type);
    }

    OSPRenderer ISPCDevice::newRenderer(const char *type)
    {
      return (OSPRenderer)Renderer::createInstance(type);
    }

    OSPGeometry ISPCDevice::newGeometry(const char *type)
    {
      return (OSPGeometry)Geometry::createInstance(type);
    }

    OSPMaterial ISPCDevice::newMaterial(const char *renderer_type,
                                        const char *material_type)
    {
      return (OSPMaterial)Material::createInstance(renderer_type,
                                                   material_type);
    }

    OSPCamera ISPCDevice::newCamera(const char *type)
    {
      return (OSPCamera)Camera::createInstance(type);
    }

    OSPVolume ISPCDevice::newVolume(const char *type)
    {
      return (OSPVolume)Volume::createInstance(type);
    }

    OSPTransferFunction ISPCDevice::newTransferFunction(const char *type)
    {
      return (OSPTransferFunction)TransferFunction::createInstance(type);
    }

    OSPLight ISPCDevice::newLight(const char *type)
    {
      return (OSPLight)Light::createInstance(type);
    }

    OSPTexture ISPCDevice::newTexture(const char *type)
    {
      return (OSPTexture)Texture::createInstance(type);
    }

    OSPGeometryInstance ISPCDevice::newGeometryInstance(OSPGeometry _geom)
    {
      auto *geom     = (Geometry *)_geom;
      auto *instance = new GeometryInstance(geom);
      return (OSPGeometryInstance)instance;
    }

    OSPVolumeInstance ISPCDevice::newVolumeInstance(OSPVolume _volume)
    {
      auto *volume   = (Volume *)_volume;
      auto *instance = new VolumeInstance(volume);
      return (OSPVolumeInstance)instance;
    }

    int ISPCDevice::loadModule(const char *name)
    {
      return loadLocalModule(name);
    }

    float ISPCDevice::renderFrame(OSPFrameBuffer _fb,
                                  OSPRenderer _renderer,
                                  OSPCamera _camera,
                                  OSPWorld _world)
    {
      auto f = renderFrameAsync(_fb, _renderer, _camera, _world);
      wait(f, OSP_FRAME_FINISHED);
      release(f);
      return getVariance(_fb);
    }

    OSPFuture ISPCDevice::renderFrameAsync(OSPFrameBuffer _fb,
                                           OSPRenderer _renderer,
                                           OSPCamera _camera,
                                           OSPWorld _world)
    {
      FrameBuffer *fb    = (FrameBuffer *)_fb;
      Renderer *renderer = (Renderer *)_renderer;
      Camera *camera     = (Camera *)_camera;
      World *world       = (World *)_world;

      fb->setCompletedEvent(OSP_NONE_FINISHED);

      auto *f = new RenderTask(
          fb, [=]() { return renderer->renderFrame(fb, camera, world); });

      return (OSPFuture)f;
    }

    int ISPCDevice::isReady(OSPFuture _task, OSPSyncEvent event)
    {
      auto *task = (QueryableTask *)_task;
      return task->isFinished(event);
    }

    void ISPCDevice::wait(OSPFuture _task, OSPSyncEvent event)
    {
      auto *task = (QueryableTask *)_task;
      task->wait(event);
    }

    void ISPCDevice::cancel(OSPFuture _task)
    {
      auto *task = (QueryableTask *)_task;
      return task->cancel();
    }

    float ISPCDevice::getProgress(OSPFuture _task)
    {
      auto *task = (QueryableTask *)_task;
      return task->getProgress();
    }

    float ISPCDevice::getVariance(OSPFrameBuffer _fb)
    {
      FrameBuffer *fb = (FrameBuffer *)_fb;
      return fb->getVariance();
    }

    void ISPCDevice::release(OSPObject _obj)
    {
      if (!_obj)
        return;
      ManagedObject *obj = (ManagedObject *)_obj;
      obj->refDec();
    }

    void ISPCDevice::setMaterial(OSPGeometryInstance _instance,
                                 OSPMaterial _material)
    {
      auto *instance = (GeometryInstance *)_instance;
      auto *material = (Material *)_material;
      instance->setMaterial(material);
    }

    OSPPickResult ISPCDevice::pick(OSPFrameBuffer _fb,
                                   OSPRenderer _renderer,
                                   OSPCamera _camera,
                                   OSPWorld _world,
                                   const vec2f &screenPos)
    {
      FrameBuffer *fb    = (FrameBuffer *)_fb;
      Renderer *renderer = (Renderer *)_renderer;
      Camera *camera     = (Camera *)_camera;
      World *world       = (World *)_world;
      return renderer->pick(fb, camera, world, screenPos);
    }

    OSP_REGISTER_DEVICE(ISPCDevice, local_device);
    OSP_REGISTER_DEVICE(ISPCDevice, local);
    OSP_REGISTER_DEVICE(ISPCDevice, default_device);
    OSP_REGISTER_DEVICE(ISPCDevice, default);

  }  // namespace api
}  // namespace ospray

extern "C" OSPRAY_DLLEXPORT void ospray_init_module_ispc() {}
