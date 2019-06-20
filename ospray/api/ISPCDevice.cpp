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
#include "common/Instance.h"
#include "common/Library.h"
#include "common/Material.h"
#include "common/Util.h"
#include "common/World.h"
#include "fb/LocalFB.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "render/LoadBalancer.h"
#include "render/RenderTask.h"
#include "render/Renderer.h"
#include "texture/Texture.h"
#include "texture/Texture2D.h"
#include "transferFunction/TransferFunction.h"
#include "volume/VolumetricModel.h"
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

    ///////////////////////////////////////////////////////////////////////////
    // ManagedObject Implementation ///////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

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

    ///////////////////////////////////////////////////////////////////////////
    // Device Implementation //////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    int ISPCDevice::loadModule(const char *name)
    {
      return loadLocalModule(name);
    }

    ///////////////////////////////////////////////////////////////////////////
    // OSPRay Data Arrays /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPData ISPCDevice::newData(size_t nitems,
                                OSPDataType format,
                                const void *init,
                                int flags)
    {
      Data *data = new Data(nitems, format, init, flags);
      return (OSPData)data;
    }

    int ISPCDevice::setRegion(OSPVolume handle,
                              const void *source,
                              const vec3i &index,
                              const vec3i &count)
    {
      Volume *volume = (Volume *)handle;
      return volume->setRegion(source, index, count);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Renderable Objects /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPLight ISPCDevice::newLight(const char *type)
    {
      return (OSPLight)Light::createInstance(type);
    }

    OSPCamera ISPCDevice::newCamera(const char *type)
    {
      return (OSPCamera)Camera::createInstance(type);
    }

    OSPGeometry ISPCDevice::newGeometry(const char *type)
    {
      return (OSPGeometry)Geometry::createInstance(type);
    }

    OSPVolume ISPCDevice::newVolume(const char *type)
    {
      return (OSPVolume)Volume::createInstance(type);
    }

    OSPGeometricModel ISPCDevice::newGeometricModel(OSPGeometry _geom)
    {
      auto *geom     = (Geometry *)_geom;
      auto *instance = new GeometricModel(geom);
      return (OSPGeometricModel)instance;
    }

    OSPVolumetricModel ISPCDevice::newVolumetricModel(OSPVolume _volume)
    {
      auto *volume   = (Volume *)_volume;
      auto *instance = new VolumetricModel(volume);
      return (OSPVolumetricModel)instance;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Instancing /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPInstance ISPCDevice::newInstance()
    {
      return (OSPInstance) new Instance;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Model Meta-Data ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPMaterial ISPCDevice::newMaterial(const char *renderer_type,
                                        const char *material_type)
    {
      return (OSPMaterial)Material::createInstance(renderer_type,
                                                   material_type);
    }

    OSPTransferFunction ISPCDevice::newTransferFunction(const char *type)
    {
      return (OSPTransferFunction)TransferFunction::createInstance(type);
    }

    OSPTexture ISPCDevice::newTexture(const char *type)
    {
      return (OSPTexture)Texture::createInstance(type);
    }

    ///////////////////////////////////////////////////////////////////////////
    // World Manipulation /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPWorld ISPCDevice::newWorld()
    {
      return (OSPWorld) new World;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Object Parameters //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void ISPCDevice::setString(OSPObject _object,
                               const char *bufName,
                               const char *s)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam<std::string>(bufName, s);
    }

    void ISPCDevice::setObject(OSPObject _target,
                               const char *bufName,
                               OSPObject _value)
    {
      ManagedObject *target = (ManagedObject *)_target;
      ManagedObject *value  = (ManagedObject *)_value;
      target->setParam(bufName, value);
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

    void ISPCDevice::setInt(OSPObject _object, const char *bufName, const int f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, f);
    }

    void ISPCDevice::setVec2f(OSPObject _object,
                              const char *bufName,
                              const vec2f &v)
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

    void ISPCDevice::setVec3f(OSPObject _object,
                              const char *bufName,
                              const vec3f &v)
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

    void ISPCDevice::setVec4f(OSPObject _object,
                              const char *bufName,
                              const vec4f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setVec4i(OSPObject _object,
                              const char *bufName,
                              const vec4i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setBox1f(OSPObject _object,
                              const char *bufName,
                              const box1f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setBox1i(OSPObject _object,
                              const char *bufName,
                              const box1i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setBox2f(OSPObject _object,
                              const char *bufName,
                              const box2f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setBox2i(OSPObject _object,
                              const char *bufName,
                              const box2i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setBox3f(OSPObject _object,
                              const char *bufName,
                              const box3f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setBox3i(OSPObject _object,
                              const char *bufName,
                              const box3i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setBox4f(OSPObject _object,
                              const char *bufName,
                              const box4f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setBox4i(OSPObject _object,
                              const char *bufName,
                              const box4i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setLinear3f(OSPObject _object,
                                 const char *bufName,
                                 const linear3f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setAffine3f(OSPObject _object,
                                 const char *bufName,
                                 const affine3f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::setVoidPtr(OSPObject _object, const char *bufName, void *v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Object + Parameter Lifetime Management /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void ISPCDevice::commit(OSPObject _object)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->commit();
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

    void ISPCDevice::release(OSPObject _obj)
    {
      if (!_obj)
        return;
      ManagedObject *obj = (ManagedObject *)_obj;
      obj->refDec();
    }

    ///////////////////////////////////////////////////////////////////////////
    // FrameBuffer Manipulation ///////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPFrameBuffer ISPCDevice::frameBufferCreate(
        const vec2i &size,
        const OSPFrameBufferFormat mode,
        const uint32 channels)
    {
      FrameBuffer::ColorBufferFormat colorBufferFormat = mode;

      FrameBuffer *fb = new LocalFrameBuffer(size, colorBufferFormat, channels);
      return (OSPFrameBuffer)fb;
    }

    OSPPixelOp ISPCDevice::newPixelOp(const char *type)
    {
      return (OSPPixelOp)PixelOp::createInstance(type);
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

    float ISPCDevice::getVariance(OSPFrameBuffer _fb)
    {
      FrameBuffer *fb = (FrameBuffer *)_fb;
      return fb->getVariance();
    }

    void ISPCDevice::resetAccumulation(OSPFrameBuffer _fb)
    {
      LocalFrameBuffer *fb = (LocalFrameBuffer *)_fb;
      fb->clear();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Frame Rendering ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPRenderer ISPCDevice::newRenderer(const char *type)
    {
      return (OSPRenderer)Renderer::createInstance(type);
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

      fb->refInc();
      renderer->refInc();
      camera->refInc();
      world->refInc();

      auto *f = new RenderTask(fb, [=]() {
        float result = renderer->renderFrame(fb, camera, world);

        fb->refDec();
        renderer->refDec();
        camera->refDec();
        world->refDec();

        return result;
      });

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
