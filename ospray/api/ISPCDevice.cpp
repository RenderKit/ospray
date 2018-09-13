// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "ISPCDevice.h"
#include "common/Model.h"
#include "common/Data.h"
#include "common/Util.h"
#include "geometry/TriangleMesh.h"
#include "render/Renderer.h"
#include "camera/Camera.h"
#include "volume/Volume.h"
#include "transferFunction/TransferFunction.h"
#include "render/LoadBalancer.h"
#include "common/Material.h"
#include "common/Library.h"
#include "texture/Texture.h"
#include "texture/Texture2D.h"
#include "lights/Light.h"
#include "fb/LocalFB.h"

// stl
#include <algorithm>

extern "C" {
  RTCDevice ispc_embreeDevice()
  {
    return ospray::api::ISPCDevice::embreeDevice;
  }
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

    static void embreeErrorFunc(void *, const RTCError code, const char* str)
    {
      postStatusMsg() << "#osp: embree internal error " << code << " : " << str;
      throw std::runtime_error("embree internal error '" +std::string(str)+"'");
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

    OSPFrameBuffer
    ISPCDevice::frameBufferCreate(const vec2i &size,
                                   const OSPFrameBufferFormat mode,
                                   const uint32 channels)
    {
      FrameBuffer::ColorBufferFormat colorBufferFormat = mode;

      FrameBuffer *fb = new LocalFrameBuffer(size, colorBufferFormat, channels);
      return (OSPFrameBuffer)fb;
    }

    void ISPCDevice::frameBufferClear(OSPFrameBuffer _fb,
                                       const uint32 fbChannelFlags)
    {
      LocalFrameBuffer *fb = (LocalFrameBuffer*)_fb;
      fb->clear(fbChannelFlags);
    }

    /*! map frame buffer */
    const void *ISPCDevice::frameBufferMap(OSPFrameBuffer _fb,
                                           OSPFrameBufferChannel channel)
    {
      LocalFrameBuffer *fb = (LocalFrameBuffer*)_fb;
      return fb->mapBuffer(channel);
    }

    /*! unmap previously mapped frame buffer */
    void ISPCDevice::frameBufferUnmap(const void *mapped,
                                       OSPFrameBuffer _fb)
    {
      FrameBuffer *fb = (FrameBuffer *)_fb;
      fb->unmap(mapped);
    }

    /*! create a new model */
    OSPModel ISPCDevice::newModel()
    {
      return (OSPModel)new Model;
    }

    /*! finalize a newly specified model */
    void ISPCDevice::commit(OSPObject _object)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->commit();
    }

    /*! add a new geometry to a model */
    void ISPCDevice::addGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Model *model = (Model *)_model;
      Geometry *geometry = (Geometry *)_geometry;
      model->geometry.push_back(geometry);
    }

    void ISPCDevice::removeGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Model *model = (Model *)_model;
      Geometry *geometry = (Geometry *)_geometry;

      auto it = std::find_if(model->geometry.begin(),
                             model->geometry.end(),
                             [&](const Ref<ospray::Geometry> &g) {
                               return geometry == &*g;
                             });

      if(it != model->geometry.end()) {
        model->geometry.erase(it);
      }
    }

    /*! add a new volume to a model */
    void ISPCDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      Model *model = (Model *) _model;
      Volume *volume = (Volume *) _volume;
      model->volume.push_back(volume);
    }

    void ISPCDevice::removeVolume(OSPModel _model, OSPVolume _volume)
    {
      Model *model = (Model *)_model;
      Volume *volume = (Volume *)_volume;

      auto it = std::find_if(model->volume.begin(),
                             model->volume.end(),
                             [&](const Ref<ospray::Volume> &g) {
                               return volume == &*g;
                             });

      if(it != model->volume.end()) {
        model->volume.erase(it);
      }
    }

    /*! create a new data buffer */
    OSPData ISPCDevice::newData(size_t nitems, OSPDataType format,
                                 const void *init, int flags)
    {
      Data *data = new Data(nitems,format,init,flags);
      return (OSPData)data;
    }

    /*! assign (named) string parameter to an object */
    void ISPCDevice::setString(OSPObject _object,
                                const char *bufName,
                                const char *s)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam<std::string>(bufName, s);
    }

    /*! assign (named) string parameter to an object */
    void ISPCDevice::setVoidPtr(OSPObject _object,
                                 const char *bufName,
                                 void *v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    void ISPCDevice::removeParam(OSPObject _object, const char *name)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->removeParam(name);
    }

    /*! assign (named) int parameter to an object */
    void ISPCDevice::setInt(OSPObject _object,
                            const char *bufName,
                            const int f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, f);
    }

    /*! assign (named) float parameter to an object */
    void ISPCDevice::setBool(OSPObject _object,
                             const char *bufName,
                             const bool b)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, b);
    }

    /*! assign (named) float parameter to an object */
    void ISPCDevice::setFloat(OSPObject _object,
                              const char *bufName,
                              const float f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, f);
    }

    /*! Copy data into the given volume. */
    int ISPCDevice::setRegion(OSPVolume handle, const void *source,
                              const vec3i &index, const vec3i &count)
    {
      Volume *volume = (Volume *)handle;
      return volume->setRegion(source, index, count);
    }

    /*! assign (named) vec2f parameter to an object */
    void ISPCDevice::setVec2f(OSPObject _object,
                              const char *bufName,
                              const vec2f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    /*! assign (named) vec3f parameter to an object */
    void ISPCDevice::setVec3f(OSPObject _object,
                              const char *bufName,
                              const vec3f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    /*! assign (named) vec3f parameter to an object */
    void ISPCDevice::setVec4f(OSPObject _object,
                              const char *bufName,
                              const vec4f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    /*! assign (named) vec2f parameter to an object */
    void ISPCDevice::setVec2i(OSPObject _object,
                              const char *bufName,
                              const vec2i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    /*! assign (named) vec3i parameter to an object */
    void ISPCDevice::setVec3i(OSPObject _object,
                              const char *bufName,
                              const vec3i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      object->setParam(bufName, v);
    }

    /*! assign (named) data item as a parameter to an object */
    void ISPCDevice::setObject(OSPObject _target,
                               const char *bufName,
                               OSPObject _value)
    {
      ManagedObject *target = (ManagedObject *)_target;
      ManagedObject *value  = (ManagedObject *)_value;
      target->setParam(bufName, value);
    }

    /*! create a new pixelOp object (out of list of registered pixelOps) */
    OSPPixelOp ISPCDevice::newPixelOp(const char *type)
    {
      return (OSPPixelOp)PixelOp::createInstance(type);
    }

    /*! set a frame buffer's pixel op object */
    void ISPCDevice::setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op)
    {
      FrameBuffer *fb = (FrameBuffer*)_fb;
      PixelOp *po = (PixelOp*)_op;
      fb->pixelOp = po->createInstance(fb,fb->pixelOp.ptr);
    }

    /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer ISPCDevice::newRenderer(const char *type)
    {
      return (OSPRenderer)Renderer::createInstance(type);
    }

    /*! create a new geometry object (out of list of registered geometrys) */
    OSPGeometry ISPCDevice::newGeometry(const char *type)
    {
      return (OSPGeometry)Geometry::createInstance(type);
    }

    OSPMaterial ISPCDevice::newMaterial(OSPRenderer _renderer,
                                        const char *material_type)
    {
      auto *renderer = reinterpret_cast<Renderer*>(_renderer);
      auto name = renderer->getParamString("externalNameFromAPI");
      return newMaterial(name.c_str(), material_type);
    }

    /*! have given renderer create a new material */
    OSPMaterial ISPCDevice::newMaterial(const char *renderer_type,
                                        const char *material_type)
    {
      return (OSPMaterial)Material::createInstance(renderer_type,
                                                   material_type);
    }

    /*! create a new camera object (out of list of registered cameras) */
    OSPCamera ISPCDevice::newCamera(const char *type)
    {
      return (OSPCamera)Camera::createInstance(type);
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPVolume ISPCDevice::newVolume(const char *type)
    {
      return (OSPVolume)Volume::createInstance(type);
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPTransferFunction ISPCDevice::newTransferFunction(const char *type)
    {
      return (OSPTransferFunction)TransferFunction::createInstance(type);
    }

    OSPLight ISPCDevice::newLight(const char *type)
    {
      return (OSPLight)Light::createInstance(type);
    }

    /*! create a new Texture object */
    OSPTexture ISPCDevice::newTexture(const char *type)
    {
      return (OSPTexture)Texture::createInstance(type);
    }

    /*! load module */
    int ISPCDevice::loadModule(const char *name)
    {
      return loadLocalModule(name);
    }

    /*! call a renderer to render a frame buffer */
    float ISPCDevice::renderFrame(OSPFrameBuffer _fb,
                                  OSPRenderer    _renderer,
                                  const uint32   fbChannelFlags)
    {
      FrameBuffer *fb       = (FrameBuffer *)_fb;
      Renderer    *renderer = (Renderer *)_renderer;

      return renderer->renderFrame(fb, fbChannelFlags);
    }

    //! release (i.e., reduce refcount of) given object
    /*! Note that all objects in ospray are refcounted, so one cannot
      explicitly "delete" any object. Instead, each object is created
      with a refcount of 1, and this refcount will be
      increased/decreased every time another object refers to this
      object resp. releases its hold on it; if the refcount is 0 the
      object will automatically get deleted. For example, you can
      create a new material, assign it to a geometry, and immediately
      after this assignation release it; the material will
      stay 'alive' as long as the given geometry requires it. */
    void ISPCDevice::release(OSPObject _obj)
    {
      if (!_obj) return;
      ManagedObject *obj = (ManagedObject *)_obj;
      obj->refDec();
    }

    //! assign given material to given geometry
    void ISPCDevice::setMaterial(OSPGeometry _geometry, OSPMaterial _material)
    {
      Geometry *geometry = (Geometry*)_geometry;
      Material *material = (Material*)_material;
      geometry->setMaterial(material);
    }

    OSPPickResult ISPCDevice::pick(OSPRenderer _renderer,
                                   const vec2f &screenPos)
    {
      Renderer *renderer = (Renderer*)_renderer;
      return renderer->pick(screenPos);
    }

    void ISPCDevice::sampleVolume(float **results,
                                  OSPVolume _volume,
                                  const vec3f *worldCoordinates,
                                  const size_t &count)
    {
      Volume *volume = (Volume *)_volume;
      volume->computeSamples(results, worldCoordinates, count);
    }

    OSP_REGISTER_DEVICE(ISPCDevice, local_device);
    OSP_REGISTER_DEVICE(ISPCDevice, local);
    OSP_REGISTER_DEVICE(ISPCDevice, default_device);
    OSP_REGISTER_DEVICE(ISPCDevice, default);

  } // ::ospray::api
} // ::ospray

extern "C" OSPRAY_DLLEXPORT void ospray_init_module_ispc()
{
}
