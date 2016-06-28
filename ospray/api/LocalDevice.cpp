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

#include "LocalDevice.h"
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/geometry/TriangleMesh.h"
#include "ospray/render/Renderer.h"
#include "ospray/camera/Camera.h"
#include "ospray/volume/Volume.h"
#include "ospray/transferFunction/TransferFunction.h"
#include "ospray/render/LoadBalancer.h"
#include "ospray/common/Material.h"
#include "ospray/common/Library.h"
#include "ospray/texture/Texture2D.h"
#include "ospray/lights/Light.h"
#include "ospray/fb/LocalFB.h"

// stl
#include <algorithm>

namespace ospray {
  extern RTCDevice g_embreeDevice;

  namespace api {

    void embreeErrorFunc(const RTCError code, const char* str)
    {
      std::cerr << "#osp: embree internal error " << code << " : " << str
                << std::endl;
      throw std::runtime_error("embree internal error '" +std::string(str)+"'");
    }

    LocalDevice::LocalDevice(int *_ac, const char **_av)
    {
      char *logLevelFromEnv = getenv("OSPRAY_LOG_LEVEL");
      if (logLevelFromEnv && logLevel == 0)
        logLevel = atoi(logLevelFromEnv);

      // -------------------------------------------------------
      // initialize embree. (we need to do this here rather than in
      // ospray::init() because in mpi-mode the latter is also called
      // in the host-stubs, where it shouldn't.
      // -------------------------------------------------------
      std::stringstream embreeConfig;
      if (debugMode)
        embreeConfig << " threads=1,verbose=2";
      else if(numThreads > 0)
        embreeConfig << " threads=" << numThreads;
      g_embreeDevice = rtcNewDevice(embreeConfig.str().c_str());

      rtcDeviceSetErrorFunction(g_embreeDevice, embreeErrorFunc);

      RTCError erc = rtcDeviceGetError(g_embreeDevice);
      if (erc != RTC_NO_ERROR) {
        // why did the error function not get called !?
        std::cerr << "#osp:init: embree internal error number " << (int)erc
                  << std::endl;
        assert(erc == RTC_NO_ERROR);
      }

      TiledLoadBalancer::instance = new LocalTiledLoadBalancer;
    }

    LocalDevice::~LocalDevice()
    {
      rtcDeleteDevice(g_embreeDevice);
    }

    OSPFrameBuffer
    LocalDevice::frameBufferCreate(const vec2i &size,
                                   const OSPFrameBufferFormat mode,
                                   const uint32 channels)
    {
      FrameBuffer::ColorBufferFormat colorBufferFormat = mode;
      bool hasDepthBuffer = (channels & OSP_FB_DEPTH)!=0;
      bool hasAccumBuffer = (channels & OSP_FB_ACCUM)!=0;
      bool hasVarianceBuffer = (channels & OSP_FB_VARIANCE)!=0;

      FrameBuffer *fb = new LocalFrameBuffer(size,colorBufferFormat,
                                             hasDepthBuffer,hasAccumBuffer,
                                             hasVarianceBuffer);
      fb->refInc();
      return (OSPFrameBuffer)fb;
    }


      /*! clear the specified channel(s) of the frame buffer specified in
       *  'whichChannels'

        if whichChannel&OSP_FB_COLOR!=0, clear the color buffer to
        '0,0,0,0'.

        if whichChannel&OSP_FB_DEPTH!=0, clear the depth buffer to
        +inf.

        if whichChannel&OSP_FB_ACCUM!=0, clear the accum buffer to 0,0,0,0,
        and reset accumID.
      */
    void LocalDevice::frameBufferClear(OSPFrameBuffer _fb,
                                       const uint32 fbChannelFlags)
    {
      LocalFrameBuffer *fb = (LocalFrameBuffer*)_fb;
      fb->clear(fbChannelFlags);
    }


    /*! map frame buffer */
    const void *LocalDevice::frameBufferMap(OSPFrameBuffer _fb,
                                            OSPFrameBufferChannel channel)
    {
      LocalFrameBuffer *fb = (LocalFrameBuffer*)_fb;
      switch (channel) {
      case OSP_FB_COLOR: return fb->mapColorBuffer();
      case OSP_FB_DEPTH: return fb->mapDepthBuffer();
      default: return nullptr;
      }
    }

    /*! unmap previously mapped frame buffer */
    void LocalDevice::frameBufferUnmap(const void *mapped,
                                       OSPFrameBuffer _fb)
    {
      Assert2(_fb != nullptr, "invalid framebuffer");
      FrameBuffer *fb = (FrameBuffer *)_fb;
      fb->unmap(mapped);
    }

    /*! create a new model */
    OSPModel LocalDevice::newModel()
    {
      Model *model = new Model;
      model->refInc();
      return (OSPModel)model;
    }

    /*! finalize a newly specified model */
    void LocalDevice::commit(OSPObject _object)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert2(object,"null object in LocalDevice::commit()");
      object->commit();

      // hack, to stay compatible with earlier version
      Model *model = dynamic_cast<Model *>(object);
      if (model)
        model->finalize();
    }

    /*! add a new geometry to a model */
    void LocalDevice::addGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Model *model = (Model *)_model;
      Assert2(model,"null model in LocalDevice::addModel()");

      Geometry *geometry = (Geometry *)_geometry;
      Assert2(geometry,"null geometry in LocalDevice::addGeometry()");

      model->geometry.push_back(geometry);
    }

    void LocalDevice::removeGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Model *model = (Model *)_model;
      Assert2(model, "null model in LocalDevice::removeGeometry");

      Geometry *geometry = (Geometry *)_geometry;
      Assert2(geometry, "null geometry in LocalDevice::removeGeometry");

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
    void LocalDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      Model *model = (Model *) _model;
      Assert2(model, "null model in LocalDevice::addVolume()");

      Volume *volume = (Volume *) _volume;
      Assert2(volume, "null volume in LocalDevice::addVolume()");

      model->volume.push_back(volume);
    }

    void LocalDevice::removeVolume(OSPModel _model, OSPVolume _volume)
    {
      Model *model = (Model *)_model;
      Assert2(model, "null model in LocalDevice::removeVolume");

      Volume *volume = (Volume *)_volume;
      Assert2(volume, "null volume in LocalDevice::removeVolume");

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
    OSPData LocalDevice::newData(size_t nitems, OSPDataType format,
                                 void *init, int flags)
    {
      Data *data = new Data(nitems,format,init,flags);
      data->refInc();
      return (OSPData)data;
    }

    /*! assign (named) string parameter to an object */
    void LocalDevice::setString(OSPObject _object,
                                const char *bufName,
                                const char *s)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != nullptr  && "invalid object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");
      object->findParam(bufName,1)->set(s);
    }

    /*! assign (named) string parameter to an object */
    void LocalDevice::setVoidPtr(OSPObject _object,
                                 const char *bufName,
                                 void *v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != nullptr  && "invalid object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");
      object->findParam(bufName,1)->set(v);
    }

    /*! assign (named) int parameter to an object */
    void LocalDevice::setInt(OSPObject _object,
                             const char *bufName,
                             const int f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != nullptr  && "invalid object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(f);
    }
    /*! assign (named) float parameter to an object */
    void LocalDevice::setFloat(OSPObject _object,
                               const char *bufName,
                               const float f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != nullptr  && "invalid object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");

      ManagedObject::Param *param = object->findParam(bufName,1);
      param->set(f);
    }

    /*! Copy data into the given volume. */
    int LocalDevice::setRegion(OSPVolume handle, const void *source,
                               const vec3i &index, const vec3i &count)
    {
      Volume *volume = (Volume *) handle;
      Assert(volume != nullptr && "invalid volume object handle");
      return(volume->setRegion(source, index, count));
    }

    /*! assign (named) vec2f parameter to an object */
    void LocalDevice::setVec2f(OSPObject _object,
                               const char *bufName,
                               const vec2f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != nullptr  && "invalid object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");

      object->findParam(bufName, 1)->set(v);
    }

    /*! assign (named) vec3f parameter to an object */
    void LocalDevice::setVec3f(OSPObject _object,
                               const char *bufName,
                               const vec3f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != nullptr  && "invalid object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(v);
    }

    /*! assign (named) vec3f parameter to an object */
    void LocalDevice::setVec4f(OSPObject _object,
                               const char *bufName,
                               const vec4f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != nullptr  && "invalid object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(v);
    }

    /*! assign (named) vec2f parameter to an object */
    void LocalDevice::setVec2i(OSPObject _object,
                               const char *bufName,
                               const vec2i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != nullptr  && "invalid object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");

      object->findParam(bufName, 1)->set(v);
    }

    /*! assign (named) vec3i parameter to an object */
    void LocalDevice::setVec3i(OSPObject _object,
                               const char *bufName,
                               const vec3i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != nullptr  && "invalid object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(v);
    }

    /*! assign (named) data item as a parameter to an object */
    void LocalDevice::setObject(OSPObject _target,
                                const char *bufName,
                                OSPObject _value)
    {
      ManagedObject *target = (ManagedObject *)_target;
      ManagedObject *value  = (ManagedObject *)_value;

      Assert(target != nullptr  && "invalid target object handle");
      Assert(bufName != nullptr && "invalid identifier for object parameter");

      target->setParam(bufName,value);
    }

    /*! create a new pixelOp object (out of list of registered pixelOps) */
    OSPPixelOp LocalDevice::newPixelOp(const char *type)
    {
      Assert(type != nullptr && "invalid render type identifier");
      PixelOp *pixelOp = PixelOp::createPixelOp(type);
      if (!pixelOp) {
        if (ospray::debugMode) {
          throw std::runtime_error("unknown pixelOp type '" +
                                   std::string(type) + "'");
        }
        else
          return nullptr;
      }
      pixelOp->refInc();
      return (OSPPixelOp)pixelOp;
    }

    /*! set a frame buffer's pixel op object */
    void LocalDevice::setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op)
    {
      FrameBuffer *fb = (FrameBuffer*)_fb;
      PixelOp *po = (PixelOp*)_op;
      assert(fb);
      assert(po);
      fb->pixelOp = po->createInstance(fb,fb->pixelOp.ptr);
    }

    /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer LocalDevice::newRenderer(const char *type)
    {
      Assert(type != nullptr && "invalid render type identifier");
      Renderer *renderer = Renderer::createRenderer(type);
      if (!renderer) {
        if (ospray::debugMode) {
          throw std::runtime_error("unknown renderer type '" +
                                   std::string(type) + "'");
        }
        else
          return nullptr;
      }
      renderer->refInc();
      return (OSPRenderer)renderer;
    }

    /*! create a new geometry object (out of list of registered geometrys) */
    OSPGeometry LocalDevice::newGeometry(const char *type)
    {
      Assert(type != nullptr && "invalid render type identifier");
      Geometry *geometry = Geometry::createGeometry(type);
      if (!geometry) return nullptr;
      geometry->refInc();
      return (OSPGeometry)geometry;
    }

      /*! have given renderer create a new material */
    OSPMaterial LocalDevice::newMaterial(OSPRenderer _renderer,
                                         const char *type)
    {
      Assert2(type != nullptr, "invalid material type identifier");

      // -------------------------------------------------------
      // first, check if there's a renderer that we can ask to create the
      // material.
      //
      Renderer *renderer = (Renderer *)_renderer;
      if (renderer) {
        Material *material = renderer->createMaterial(type);
        if (material) {
          material->refInc();
          return (OSPMaterial)material;
        }
      }

      // -------------------------------------------------------
      // if there was no renderer, check if there's a loadable material by that
      // name
      //
      Material *material = Material::createMaterial(type);
      if (!material) return nullptr;
      material->refInc();
      return (OSPMaterial)material;
    }

    /*! create a new camera object (out of list of registered cameras) */
    OSPCamera LocalDevice::newCamera(const char *type)
    {
      Assert(type != nullptr && "invalid camera type identifier");
      Camera *camera = Camera::createCamera(type);
      if (!camera) {
        if (ospray::debugMode) {
          throw std::runtime_error("unknown camera type '"
                                   + std::string(type) + "'");
        }
        else
          return nullptr;
      }
      camera->refInc();
      return (OSPCamera)camera;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPVolume LocalDevice::newVolume(const char *type)
    {
      Assert(type != nullptr && "invalid volume type identifier");
      Volume *volume = Volume::createInstance(type);
      if (!volume) {
        if (ospray::debugMode) {
          throw std::runtime_error("unknown volume type '" +
                                   std::string(type) + "'");
        }
        else
          return nullptr;
      }
      volume->refInc();
      return (OSPVolume)volume;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPTransferFunction LocalDevice::newTransferFunction(const char *type)
    {
      Assert(type != nullptr && "invalid transfer function type identifier");
      auto *transferFunction = TransferFunction::createInstance(type);
      if (!transferFunction) {
        if (ospray::debugMode) {
          throw std::runtime_error("unknown transfer function type '" +
                                   std::string(type) + "'");
        }
        else
          return nullptr;
      }
      transferFunction->refInc();
      return (OSPTransferFunction)transferFunction;
    }

    /*! have given renderer create a new Light */
    OSPLight LocalDevice::newLight(OSPRenderer _renderer, const char *type) {
      Renderer  *renderer = (Renderer *)_renderer;
      if (renderer) {
        Light *light = renderer->createLight(type);
        if (light) {
          light->refInc();
          return (OSPLight)light;
        }
      }

      // If there was no renderer try to see if there is a loadable light by
      // that name
      Light *light = Light::createLight(type);
      if (!light) return nullptr;
      light->refInc();
      return (OSPLight)light;
    }

    /*! create a new Texture2D object */
    OSPTexture2D LocalDevice::newTexture2D(const vec2i &size,
        const OSPTextureFormat type, void *data, const uint32 flags)
    {
      Assert(size.x > 0 &&
             "Width must be greater than 0 in LocalDevice::newTexture2D");
      Assert(size.y > 0 &&
             "Height must be greater than 0 in LocalDevice::newTexture2D");
      Texture2D *tx = Texture2D::createTexture(size, type, data, flags);
      if(tx) tx->refInc();
      return (OSPTexture2D)tx;
    }

    /*! load module */
    int LocalDevice::loadModule(const char *name)
    {
      std::string libName = "ospray_module_" + std::string(name);
      loadLibrary(libName);

      std::string initSymName = "ospray_init_module_" + std::string(name);
      void*initSym = getSymbol(initSymName);
      if (!initSym) {
        throw std::runtime_error("#osp:api: could not find module initializer "
                                 +initSymName);
      }

      void (*initMethod)() = (void(*)())initSym;

      //NOTE(jda) - don't use magic numbers!
      if (!initMethod)
        return 2;

      try {
        initMethod();
      } catch (...) {
        return 3;
      }

      return 0;
    }


    /*! call a renderer to render a frame buffer */
    float LocalDevice::renderFrame(OSPFrameBuffer _fb,
                                   OSPRenderer    _renderer,
                                   const uint32   fbChannelFlags)
    {
      FrameBuffer *fb       = (FrameBuffer *)_fb;
      Renderer    *renderer = (Renderer *)_renderer;

      Assert(fb != nullptr && "invalid frame buffer handle");
      Assert(renderer != nullptr && "invalid renderer handle");

      try {
        return renderer->renderFrame(fb, fbChannelFlags);
      } catch (const std::runtime_error &e) {
        std::cerr << "======================================================="
                  << std::endl;
        std::cerr << "# >>> ospray fatal error <<< " << std::endl << e.what()
                  << std::endl;
        std::cerr << "======================================================="
                  << std::endl;
        exit(1);
      }
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
    void LocalDevice::release(OSPObject _obj)
    {
      if (!_obj) return;
      ManagedObject *obj = (ManagedObject *)_obj;
      obj->refDec();
    }

    //! assign given material to given geometry
    void LocalDevice::setMaterial(OSPGeometry _geometry, OSPMaterial _material)
    {
      Geometry *geometry = (Geometry*)_geometry;
      Material *material = (Material*)_material;
      geometry->setMaterial(material);
    }

    OSPPickResult LocalDevice::pick(OSPRenderer _renderer,
                                    const vec2f &screenPos)
    {
      Assert(_renderer != nullptr && "invalid renderer handle");
      Renderer *renderer = (Renderer*)_renderer;

      return renderer->pick(screenPos);
    }

    void LocalDevice::sampleVolume(float **results,
                                   OSPVolume _volume,
                                   const vec3f *worldCoordinates,
                                   const size_t &count)
    {
      Volume *volume = (Volume *)_volume;

      Assert2(volume, "invalid volume handle");
      Assert2(worldCoordinates, "invalid worldCoordinates");

      volume->computeSamples(results, worldCoordinates, count);
    }

  } // ::ospray::api
} // ::ospray

