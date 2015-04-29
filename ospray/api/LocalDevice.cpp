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
#include "ospray/common/TaskSys.h"
#include "ospray/fb/LocalFB.h"

// stl
#include <algorithm>

namespace ospray {
  namespace api {

    void embreeErrorFunc(const RTCError code, const char* str)
    {
      std::cerr << "#osp: embree internal error " << code << " : " << str << std::endl;
      throw std::runtime_error("embree internal error '"+std::string(str)+"'");
    }

    LocalDevice::LocalDevice(int *_ac, const char **_av)
    {
      char *logLevelFromEnv = getenv("OSPRAY_LOG_LEVEL");
      if (logLevelFromEnv) 
        logLevel = atoi(logLevelFromEnv);
      else
        logLevel = 0;

      ospray::init(_ac,&_av);

      // initialize embree. (we need to do this here rather than in
      // ospray::init() because in mpi-mode the latter is also called
      // in the host-stubs, where it shouldn't.

      rtcSetErrorFunction(embreeErrorFunc);

      // -------------------------------------------------------
      // initialize embree
      // ------------------------------------------------------- 
     std::stringstream embreeConfig;
      if (debugMode)
        embreeConfig << " threads=1,verbose=2";
      else if(numThreads > 0)
        embreeConfig << " threads=" << numThreads;
      rtcInit(embreeConfig.str().c_str());

      RTCError erc = rtcGetError();
      if (erc != RTC_NO_ERROR) {
        // why did the error function not get called !?
        std::cerr << "#osp:init: embree internal error number " << (int)erc << std::endl;
        assert(erc == RTC_NO_ERROR);
      }

      // -------------------------------------------------------
      // initialize our task system
      // -------------------------------------------------------
      if (debugMode)
        ospray::Task::initTaskSystem(0);
      else
        ospray::Task::initTaskSystem(-1);

      TiledLoadBalancer::instance = new LocalTiledLoadBalancer;
    }



    OSPFrameBuffer 
    LocalDevice::frameBufferCreate(const vec2i &size, 
                                   const OSPFrameBufferFormat mode,
                                   const uint32 channels)
    {
      FrameBuffer::ColorBufferFormat colorBufferFormat = mode; //FrameBuffer::RGBA_UINT8;//FLOAT32;
      bool hasDepthBuffer = (channels & OSP_FB_DEPTH)!=0;
      bool hasAccumBuffer = (channels & OSP_FB_ACCUM)!=0;
      
      FrameBuffer *fb = new LocalFrameBuffer(size,colorBufferFormat,
                                             hasDepthBuffer,hasAccumBuffer);
      fb->refInc();
      return (OSPFrameBuffer)fb;
    }
    

      /*! clear the specified channel(s) of the frame buffer specified in 'whichChannels'
        
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
      default: return NULL;
      }
    }

    /*! unmap previously mapped frame buffer */
    void LocalDevice::frameBufferUnmap(const void *mapped,
                                       OSPFrameBuffer _fb)
    {
      Assert2(_fb != NULL, "invalid framebuffer");
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

    /*! remove an existing geometry from a model */
    struct GeometryLocator {
      bool operator()(const embree::Ref<ospray::Geometry> &g) const {
        return ptr == &*g;
      }
      Geometry *ptr;
    };

    void LocalDevice::removeGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Model *model = (Model *)_model;
      Assert2(model, "null model in LocalDevice::removeGeometry");

      Geometry *geometry = (Geometry *)_geometry;
      Assert2(model, "null geometry in LocalDevice::removeGeometry");

      GeometryLocator locator;
      locator.ptr = geometry;
      Model::GeometryVector::iterator it = std::find_if(model->geometry.begin(), model->geometry.end(), locator);
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

      model->volumes.push_back(volume);
    }

    /*! create a new data buffer */
    OSPTriangleMesh LocalDevice::newTriangleMesh()
    {
      TriangleMesh *triangleMesh = new TriangleMesh;
      triangleMesh->refInc();
      return (OSPTriangleMesh)triangleMesh;
    }

    /*! create a new data buffer */
    OSPData LocalDevice::newData(size_t nitems, OSPDataType format, void *init, int flags)
    {
      Data *data = new Data(nitems,format,init,flags);
      data->refInc();
      return (OSPData)data;
    }
    
    /*! assign (named) string parameter to an object */
    void LocalDevice::setString(OSPObject _object, const char *bufName, const char *s)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");
      object->findParam(bufName,1)->set(s);
    }

    /*! assign (named) string parameter to an object */
    void LocalDevice::setVoidPtr(OSPObject _object, const char *bufName, void *v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");
      object->findParam(bufName,1)->set(v);
    }

    /*! assign (named) int parameter to an object */
    void LocalDevice::setInt(OSPObject _object, const char *bufName, const int f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(f);
    }
    /*! assign (named) float parameter to an object */
    void LocalDevice::setFloat(OSPObject _object, const char *bufName, const float f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");
      
      ManagedObject::Param *param = object->findParam(bufName,1);
      param->set(f);
    }

    /*! Copy data into the given volume. */
    int LocalDevice::setRegion(OSPVolume handle, const void *source, 
                               const vec3i &index, const vec3i &count) 
    {
      Volume *volume = (Volume *) handle;
      Assert(volume != NULL && "invalid volume object handle");
      return(volume->setRegion(source, index, count));
    }

    /*! assign (named) vec2f parameter to an object */
    void LocalDevice::setVec2f(OSPObject _object, const char *bufName, const vec2f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName, 1)->set(v);
    }

    /*! assign (named) vec3f parameter to an object */
    void LocalDevice::setVec3f(OSPObject _object, const char *bufName, const vec3f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(v);
    }
    /*! assign (named) vec3i parameter to an object */
    void LocalDevice::setVec3i(OSPObject _object, const char *bufName, const vec3i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(v);
    }

    /*! assign (named) data item as a parameter to an object */
    void LocalDevice::setObject(OSPObject _target, const char *bufName, OSPObject _value)
    {
      ManagedObject *target = (ManagedObject *)_target;
      ManagedObject *value  = (ManagedObject *)_value;

      Assert(target != NULL  && "invalid target object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      target->setParam(bufName,value);
    }

    /*! Get the handle of the named data array associated with an object. */
    int LocalDevice::getData(OSPObject handle, const char *name, OSPData *value) 
    {
      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      ManagedObject::Param *param = object->findParam(name);
      return(param && param->type == OSP_OBJECT && param->ptr->managedObjectType == OSP_DATA ? *value = (OSPData) param->ptr, true : false);
    }

    /*! Get a copy of the data in an array (the application is responsible for freeing this pointer). */
    int LocalDevice::getDataValues(OSPData handle, void **pointer, size_t *count, OSPDataType *type)
    {
      Data *data = (Data *) handle;
      Assert(data != NULL && "invalid data object handle");
     *pointer = malloc(data->numBytes);  if (pointer == NULL) return(false);
      return(memcpy(*pointer, data->data, data->numBytes), *count = data->numItems, *type = data->type, true);
    }

    /*! Get the named scalar floating point value associated with an object. */
    int LocalDevice::getf(OSPObject handle, const char *name, float *value) 
    {
      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      ManagedObject::Param *param = object->findParam(name);
      return(param && param->type == OSP_FLOAT ? *value = param->f[0], true : false);
    }

    /*! Get the named scalar integer associated with an object. */
    int LocalDevice::geti(OSPObject handle, const char *name, int *value) 
    {
      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      ManagedObject::Param *param = object->findParam(name);
      return(param && param->type == OSP_INT ? *value = param->i[0], true : false);
    }

    /*! Get the material associated with a geometry object. */
    int LocalDevice::getMaterial(OSPGeometry handle, OSPMaterial *value) 
    {
      Geometry *geometry = (Geometry *) handle;
      Assert(geometry != NULL && "invalid source geometry handle");
      return(*value = (OSPMaterial) geometry->getMaterial(), true);
    }

    /*! Get the named object associated with an object. */
    int LocalDevice::getObject(OSPObject handle, const char *name, OSPObject *value) 
    {
      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      ManagedObject::Param *param = object->findParam(name);
      return(param && param->type == OSP_OBJECT ? *value = (OSPObject) param->ptr, true : false);
    }

    /*! Retrieve a NULL-terminated list of the parameter names associated with an object. */
    int LocalDevice::getParameters(OSPObject handle, char ***value) 
    {
      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      char **names = (char **) malloc((object->paramList.size() + 1) * sizeof(char *));

      for (size_t i=0 ; i < object->paramList.size() ; i++) 
        names[i] = strdup(object->paramList[i]->name);
      names[object->paramList.size()] = NULL;
      return(*value = names, true);

    }

    /*! Get a pointer to a copy of the named character string associated with an object. */
    int LocalDevice::getString(OSPObject handle, const char *name, char **value) {

      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      ManagedObject::Param *param = object->findParam(name);
      return(param && param->type == OSP_STRING ? *value = strdup(param->s), true : false);
    }

    /*! Get the type of the named parameter or the given object (if 'name' is NULL). */
    int LocalDevice::getType(OSPObject handle, const char *name, OSPDataType *value) 
    {
      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      if (name == NULL) return(*value = object->managedObjectType, true);

      ManagedObject::Param *param = object->findParam(name);
      if (param == NULL) return(false);
      return(*value
             = (param->type == OSP_OBJECT) 
             ?  param->ptr->managedObjectType
             :  param->type, true);
    }

    /*! Get the named 2-vector floating point value associated with an object. */
    int LocalDevice::getVec2f(OSPObject handle, const char *name, vec2f *value) {

      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      ManagedObject::Param *param = object->findParam(name);
      return(param && param->type == OSP_FLOAT2 ? *value = ((vec2f *) param->f)[0], true : false);
    }

    /*! Get the named 3-vector floating point value associated with an object. */
    int LocalDevice::getVec3f(OSPObject handle, const char *name, vec3f *value) 
    {
      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      ManagedObject::Param *param = object->findParam(name);
      return(param && param->type == OSP_FLOAT3 ? *value = ((vec3f *) param->f)[0], true : false);
    }

    /*! Get the named 3-vector integer value associated with an object. */
    int LocalDevice::getVec3i(OSPObject handle, const char *name, vec3i *value) 
    {
      ManagedObject *object = (ManagedObject *) handle;
      Assert(object != NULL && "invalid source object handle");
      ManagedObject::Param *param = object->findParam(name);
      return(param && param->type == OSP_INT3 ? *value = ((vec3i *) param->i)[0], true : false);
    }

    /*! create a new pixelOp object (out of list of registered pixelOps) */
    OSPPixelOp LocalDevice::newPixelOp(const char *type)
    {
      Assert(type != NULL && "invalid render type identifier");
      PixelOp *pixelOp = PixelOp::createPixelOp(type);
      if (!pixelOp) {
        if (ospray::debugMode) 
          throw std::runtime_error("unknown pixelOp type '"+std::string(type)+"'");
        else
          return NULL;
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
      Assert(type != NULL && "invalid render type identifier");
      Renderer *renderer = Renderer::createRenderer(type);
      if (!renderer) {
        if (ospray::debugMode) 
          throw std::runtime_error("unknown renderer type '"+std::string(type)+"'");
        else
          return NULL;
      }
      renderer->refInc();
      return (OSPRenderer)renderer;
    }

    /*! create a new geometry object (out of list of registered geometrys) */
    OSPGeometry LocalDevice::newGeometry(const char *type)
    {
      Assert(type != NULL && "invalid render type identifier");
      Geometry *geometry = Geometry::createGeometry(type);
      if (!geometry) return NULL;
      geometry->refInc();
      return (OSPGeometry)geometry;
    }

      /*! have given renderer create a new material */
    OSPMaterial LocalDevice::newMaterial(OSPRenderer _renderer, const char *type)
    {
      Assert2(type != NULL, "invalid material type identifier");

      // -------------------------------------------------------
      // first, check if there's a renderer that we can ask to create the material.
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
      // if there was no renderer, check if there's a loadable material by that name
      //
      Material *material = Material::createMaterial(type);
      if (!material) return NULL;
      material->refInc();
      return (OSPMaterial)material;
    }

    /*! create a new camera object (out of list of registered cameras) */
    OSPCamera LocalDevice::newCamera(const char *type)
    {
      Assert(type != NULL && "invalid camera type identifier");
      Camera *camera = Camera::createCamera(type);
      if (!camera) {
        if (ospray::debugMode) 
          throw std::runtime_error("unknown camera type '"+std::string(type)+"'");
        else
          return NULL;
      }
      camera->refInc();
      return (OSPCamera)camera;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPVolume LocalDevice::newVolume(const char *type)
    {
      Assert(type != NULL && "invalid volume type identifier");
      Volume *volume = Volume::createInstance(type);
      if (!volume) {
        if (ospray::debugMode) 
          throw std::runtime_error("unknown volume type '"+std::string(type)+"'");
        else
          return NULL;
      }
      volume->refInc();
      return (OSPVolume)volume;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPTransferFunction LocalDevice::newTransferFunction(const char *type)
    {
      Assert(type != NULL && "invalid transfer function type identifier");
      TransferFunction *transferFunction = TransferFunction::createInstance(type);
      if (!transferFunction) {
        if (ospray::debugMode)
          throw std::runtime_error("unknown transfer function type '"+std::string(type)+"'");
        else
          return NULL;
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

      //If there was no renderer try to see if there is a loadable light by that name
      Light *light = Light::createLight(type);
      if (!light) return NULL;
      light->refInc();
      return (OSPLight)light;
    }

    /*! create a new Texture2D object */
    OSPTexture2D LocalDevice::newTexture2D(int width, int height, OSPDataType type, void *data, int flags) {
      Assert(width > 0 && "Width must be greater than 0 in LocalDevice::newTexture2D");
      Assert(height > 0 && "Height must be greater than 0 in LocalDevice::newTexture2D");
      Texture2D *tx = Texture2D::createTexture(width, height, type, data, flags);
      if(tx) tx->refInc();
      return (OSPTexture2D)tx;
    }

    /*! load module */
    int LocalDevice::loadModule(const char *name)
    {
      std::string libName = "ospray_module_"+std::string(name);
      loadLibrary(libName);
      
      std::string initSymName = "ospray_init_module_"+std::string(name);
      void*initSym = getSymbol(initSymName);
      if (!initSym)
        throw std::runtime_error("#osp:api: could not find module initializer "+initSymName);
      void (*initMethod)() = (void(*)())initSym;
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
    void LocalDevice::renderFrame(OSPFrameBuffer _fb, 
                                  OSPRenderer    _renderer, 
                                  const uint32 fbChannelFlags)
    {
      FrameBuffer *fb       = (FrameBuffer *)_fb;
      // SwapChain *sc       = (SwapChain *)_sc;
      Renderer  *renderer = (Renderer *)_renderer;
      // Model *model = (Model *)_model;

      Assert(fb != NULL && "invalid frame buffer handle");
      // Assert(sc != NULL && "invalid frame buffer handle");
      Assert(renderer != NULL && "invalid renderer handle");
      
      // FrameBuffer *fb = sc->getBackBuffer();
      renderer->renderFrame(fb,fbChannelFlags);
      // WARNING: I'm doing an *im*plicit swapbuffers here at the end
      // of renderframe, but to be more opengl-conform we should
      // actually have the user call an *ex*plicit ospSwapBuffers call...
      // sc->advance();
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

    OSPPickResult LocalDevice::pick(OSPRenderer _renderer, const vec2f &screenPos)
    {
      Assert(_renderer != NULL && "invalid renderer handle");
      Renderer *renderer = (Renderer*)_renderer;

      return renderer->pick(screenPos);
    }

  } // ::ospray::api
} // ::ospray

