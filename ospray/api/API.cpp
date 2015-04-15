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

#include "ospray/include/ospray/ospray.h"
#include "ospray/render/Renderer.h"
#include "ospray/camera/Camera.h"
#include "ospray/common/Material.h"
#include "ospray/volume/Volume.h"
#include "ospray/transferFunction/TransferFunction.h"
#include "LocalDevice.h"
#include "ospray/common/OSPCommon.h"

#if 1
# define LOG(a) if (ospray::logLevel > 2) std::cout << "#ospray: " << a << std::endl;
#else
# define LOG(a) /*ignore*/
#endif

/*! \file api.cpp implements the public ospray api functions by
  routing them to a respective \ref device */
namespace ospray {

  using std::endl;
  using std::cout;

#if OSPRAY_MPI
  namespace mpi {
    ospray::api::Device *createMPI_ListenForWorkers(int *ac, const char **av, 
                                                    const char *fileNameToStorePortIn);
    ospray::api::Device *createMPI_LaunchWorkerGroup(int *ac, const char **av, 
                                                     const char *launchCommand);
    ospray::api::Device *createMPI_RanksBecomeWorkers(int *ac, const char **av);
  }
#endif
#if OSPRAY_MIC_COI
  namespace coi {
    ospray::api::Device *createCoiDevice(int *ac, const char **av);
  }
#endif


#define ASSERT_DEVICE() if (ospray::api::Device::current == NULL)     \
    throw std::runtime_error("OSPRay not yet initialized "            \
                             "(most likely this means you tried to "  \
                             "call an ospray API function before "    \
                             "first calling ospInit())");

  
  extern "C" void ospInit(int *_ac, const char **_av) 
  {
    if (ospray::api::Device::current) 
      throw std::runtime_error("OSPRay error: device already exists "
                               "(did you call ospInit twice?)");

    /* call ospray::init to properly parse common args like
       --osp:verbose, --osp:debug etc */
    ospray::init(_ac,&_av);

    const char *OSP_MPI_LAUNCH_FROM_ENV = getenv("OSPRAY_MPI_LAUNCH");

    if (OSP_MPI_LAUNCH_FROM_ENV) {
#if OSPRAY_MPI
      std::cout << "#osp: launching ospray mpi ring - make sure that mpd is running" << std::endl;
      ospray::api::Device::current
        = mpi::createMPI_LaunchWorkerGroup(_ac,_av,OSP_MPI_LAUNCH_FROM_ENV);
#else
      throw std::runtime_error("OSPRay MPI support not compiled in");
#endif
    }

    if (_ac && _av) {
      // we're only supporting local rendering for now - network device
      // etc to come.
      for (int i=1;i<*_ac;i++) {

        if (std::string(_av[i]) == "--osp:mpi") {
#if OSPRAY_MPI
          removeArgs(*_ac,(char **&)_av,i,1);
          ospray::api::Device::current
            = mpi::createMPI_RanksBecomeWorkers(_ac,_av);
#else
          throw std::runtime_error("OSPRay MPI support not compiled in");
#endif
          --i; continue;
        }
        if (std::string(_av[i]) == "--osp:coi") {
#if OSPRAY_TARGET_MIC
          throw std::runtime_error("The COI device can only be created on the host");
#elif OSPRAY_MIC_COI
          removeArgs(*_ac,(char **&)_av,i,1);
          ospray::api::Device::current
            = ospray::coi::createCoiDevice(_ac,_av);
#else
          throw std::runtime_error("OSPRay's COI support not compiled in");
#endif
          --i; continue;
        }

        if (std::string(_av[i]) == "--osp:mpi-launch") {
#if OSPRAY_MPI
          if (i+2 > *_ac)
            throw std::runtime_error("--osp:mpi-launch expects an argument");
          const char *launchCommand = strdup(_av[i+1]);
          removeArgs(*_ac,(char **&)_av,i,2);
          ospray::api::Device::current
            = mpi::createMPI_LaunchWorkerGroup(_ac,_av,launchCommand);
#else
          throw std::runtime_error("OSPRay MPI support not compiled in");
#endif
          --i; continue;
        }
        const char *listenArgName = "--osp:mpi-listen";
        if (!strncmp(_av[i],listenArgName,strlen(listenArgName))) {
#if OSPRAY_MPI
          const char *fileNameToStorePortIn = NULL;
          if (strlen(_av[i]) > strlen(listenArgName)) {
            fileNameToStorePortIn = strdup(_av[i]+strlen(listenArgName)+1);
          }
          removeArgs(*_ac,(char **&)_av,i,1);
          ospray::api::Device::current
            = mpi::createMPI_ListenForWorkers(_ac,_av,fileNameToStorePortIn);
#else
          throw std::runtime_error("OSPRay MPI support not compiled in");
#endif
          --i; continue;
        }
      }
    }
    
    // no device created on cmd line, yet, so default to localdevice
    if (ospray::api::Device::current == NULL)
      ospray::api::Device::current = new ospray::api::LocalDevice(_ac,_av);
  } 


  /*! destroy a given frame buffer. 

    due to internal reference counting the framebuffer may or may not be deleted immediately
  */
  extern "C" void ospFreeFrameBuffer(OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(fb != NULL);
    ospray::api::Device::current->release(fb);
  }

  extern "C" OSPFrameBuffer ospNewFrameBuffer(const osp::vec2i &size, 
                                              const OSPFrameBufferFormat mode,
                                              const int channels)
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->frameBufferCreate(size,mode,channels);
  }

  //! load module \<name\> from shard lib libospray_module_\<name\>.so, or 
  extern "C" error_t ospLoadModule(const char *moduleName)
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->loadModule(moduleName);
  }

  extern "C" const void *ospMapFrameBuffer(OSPFrameBuffer fb, 
                                           OSPFrameBufferChannel channel)
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->frameBufferMap(fb,channel);
  }
  
  extern "C" void ospUnmapFrameBuffer(const void *mapped,
                                      OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(mapped != NULL && "invalid mapped pointer in ospUnmapFrameBuffer");
    ospray::api::Device::current->frameBufferUnmap(mapped,fb);
  }

  extern "C" OSPModel ospNewModel()
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->newModel();
  }

  extern "C" void ospAddGeometry(OSPModel model, OSPGeometry geometry)
  {
    ASSERT_DEVICE();
    Assert(model != NULL && "invalid model in ospAddGeometry");
    Assert(geometry != NULL && "invalid geometry in ospAddGeometry");
    return ospray::api::Device::current->addGeometry(model,geometry);
  }

  extern "C" void ospAddVolume(OSPModel model, OSPVolume volume)
  {
    ASSERT_DEVICE();
    Assert(model != NULL && "invalid model in ospAddVolume");
    Assert(volume != NULL && "invalid volume in ospAddVolume");
    return ospray::api::Device::current->addVolume(model, volume);
  }

  extern "C" void ospRemoveGeometry(OSPModel model, OSPGeometry geometry)
  {
    ASSERT_DEVICE();
    Assert(model != NULL && "invalid model in ospRemoveGeometry");
    Assert(geometry != NULL && "invalid geometry in ospRemoveGeometry");
    return ospray::api::Device::current->removeGeometry(model, geometry);
  }

  /*! create a newa data buffer, with optional init data and control flags */
  extern "C" OSPTriangleMesh ospNewTriangleMesh()
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->newTriangleMesh();
  }

  /*! create a new data buffer, with optional init data and control flags */
  extern "C" OSPData ospNewData(size_t nitems, OSPDataType format, const void *init, int flags)
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->newData(nitems,format,(void*)init,flags);
  }

  /*! add a data array to another object */
  extern "C" void ospSetData(OSPObject object, const char *bufName, OSPData data)
  { 
    // assert(!rendering);
    ASSERT_DEVICE();
    LOG("ospSetData(...,\"" << bufName << "\",...)");
    return ospray::api::Device::current->setObject(object,bufName,(OSPObject)data);
  }

  /*! add a data array to another object */
  extern "C" void ospSetParam(OSPObject target, const char *bufName, OSPObject value)
  {
    ASSERT_DEVICE();
    static bool warned = false;
    if (!warned) {
      std::cout << "'ospSetParam()' has been deprecated. Please use the new naming convention of 'ospSetObject()' instead" << std::endl;
      warned = true;
    }
    LOG("ospSetData(...,\"" << bufName << "\",...)");
    return ospray::api::Device::current->setObject(target,bufName,value);
  }
  /*! add a data array to another object */
  extern "C" void ospSetPixelOp(OSPFrameBuffer fb, OSPPixelOp op)
  {
    ASSERT_DEVICE();
    LOG("ospSetPixelOp(...,...)");
    return ospray::api::Device::current->setPixelOp(fb,op);
  }

  /*! add a data array to another object */
  extern "C" void ospSetObject(OSPObject target, const char *bufName, OSPObject value)
  {
    ASSERT_DEVICE();
    LOG("ospSetObject(...,\"" << bufName << "\",...)");
    return ospray::api::Device::current->setObject(target,bufName,value);
  }

  /*! \brief create a new pixelOp of given type 

    return 'NULL' if that type is not known */
  extern "C" OSPPixelOp ospNewPixelOp(const char *_type)
  {
    ASSERT_DEVICE();
    Assert2(_type,"invalid render type identifier in ospNewPixelOp");
    LOG("ospNewPixelOp(" << _type << ")");
    int L = strlen(_type);
    char type[L+1];
    for (int i=0;i<=L;i++) {
      char c = _type[i];
      if (c == '-' || c == ':') c = '_';
      type[i] = c;
    }
    OSPPixelOp pixelOp = ospray::api::Device::current->newPixelOp(type);
    return pixelOp;
  }

  /*! \brief create a new renderer of given type 

    return 'NULL' if that type is not known */
  extern "C" OSPRenderer ospNewRenderer(const char *_type)
  {
    ASSERT_DEVICE();
    Assert2(_type,"invalid render type identifier in ospNewRenderer");
    LOG("ospNewRenderer(" << _type << ")");
    int L = strlen(_type);
    char type[L+1];
    for (int i=0;i<=L;i++) {
      char c = _type[i];
      if (c == '-' || c == ':') c = '_';
      type[i] = c;
    }
    OSPRenderer renderer = ospray::api::Device::current->newRenderer(type);
    return renderer;
  }
  
  /*! \brief create a new geometry of given type 

    return 'NULL' if that type is not known */
  extern "C" OSPGeometry ospNewGeometry(const char *type)
  {
    ASSERT_DEVICE();
    Assert(type != NULL && "invalid geometry type identifier in ospNewGeometry");
    LOG("ospNewGeometry(" << type << ")");
    OSPGeometry geometry = ospray::api::Device::current->newGeometry(type);
    // if (ospray::logLevel > 0)
    //   if (geometry) 
    //     cout << "ospNewGeometry: " << ((ospray::Geometry*)geometry)->toString() << endl;
    //   else
    //     std::cerr << "#ospray: could not create geometry '" << type << "'" << std::endl;
    return geometry;
  }

  /*! \brief create a new material of given type 

    return 'NULL' if that type is not known */
  extern "C" OSPMaterial ospNewMaterial(OSPRenderer renderer, const char *type)
  {
    ASSERT_DEVICE();
    // Assert2(renderer != NULL, "invalid renderer handle in ospNewMaterial");
    Assert2(type != NULL, "invalid material type identifier in ospNewMaterial");
    LOG("ospNewMaterial(" << renderer << ", " << type << ")");
    OSPMaterial material = ospray::api::Device::current->newMaterial(renderer, type);
    return material;
  }

  extern "C" OSPLight ospNewLight(OSPRenderer renderer, const char *type)
  {
    ASSERT_DEVICE();
    Assert2(type != NULL, "invalid light type identifier in ospNewLight");
    LOG("ospNewLight(" << renderer << ", " << type << ")");
    return ospray::api::Device::current->newLight(renderer, type);
  }

  /*! \brief create a new camera of given type 

    return 'NULL' if that type is not known */
  extern "C" OSPCamera ospNewCamera(const char *type)
  {
    ASSERT_DEVICE();
    Assert(type != NULL && "invalid camera type identifier in ospNewCamera");
    LOG("ospNewCamera(" << type << ")");
    OSPCamera camera = ospray::api::Device::current->newCamera(type);
    return camera;
  }

  extern "C" OSPTexture2D ospNewTexture2D(int width,
                                          int height,
                                          OSPDataType type,
                                          void *data = NULL,
                                          int flags = 0)
  {
    ASSERT_DEVICE();
    Assert2(width > 0, "Width must be greater than 0 in ospNewTexture2D");
    Assert2(height > 0, "Height must be greater than 0 in ospNewTexture2D");
    LOG("ospNewTexture2D( " << width << ", " << height << ", " << type << ", " << data << ", " << flags << ")");
    return ospray::api::Device::current->newTexture2D(width, height, type, data, flags);
  }

  /*! \brief create a new volume of given type, return 'NULL' if that type is not known */
  extern "C" OSPVolume ospNewVolume(const char *type)
  {
    ASSERT_DEVICE();
    Assert(type != NULL && "invalid volume type identifier in ospNewVolume");
    LOG("ospNewVolume(" << type << ")");
    OSPVolume volume = ospray::api::Device::current->newVolume(type);
    if (ospray::logLevel > 0) {
      if (volume) 
        cout << "ospNewVolume: " << ((ospray::Volume*)volume)->toString() << endl;
      else
        std::cerr << "#ospray: could not create volume '" << type << "'" << std::endl;
    }
    return volume;
  }

  /*! \brief create a new transfer function of given type
    return 'NULL' if that type is not known */
  extern "C" OSPTransferFunction ospNewTransferFunction(const char *type)
  {
    ASSERT_DEVICE();
    Assert(type != NULL && "invalid transfer function type identifier in ospNewTransferFunction");
    LOG("ospNewTransferFunction(" << type << ")");
    OSPTransferFunction transferFunction = ospray::api::Device::current->newTransferFunction(type);
    if(ospray::logLevel > 0) {
      if(transferFunction)
        cout << "ospNewTransferFunction: " << ((ospray::TransferFunction*)transferFunction)->toString() << endl;
      else
        std::cerr << "#ospray: could not create transfer function '" << type << "'" << std::endl;
    }
    return transferFunction;
  }

  extern "C" void ospFrameBufferClear(OSPFrameBuffer fb, 
                                      const uint32 fbChannelFlags)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->frameBufferClear(fb,fbChannelFlags);
  }

  /*! \brief call a renderer to render given model into given framebuffer 
    
    model _may_ be empty (though most framebuffers will expect one!) */
  extern "C" void ospRenderFrame(OSPFrameBuffer fb, 
                                 OSPRenderer renderer, 
                                 const uint32 fbChannelFlags=OSP_FB_COLOR
                                 )
  {
    ASSERT_DEVICE();
#if 0
    double t0 = ospray::getSysTime();
    ospray::api::Device::current->renderFrame(fb,renderer);
    double t_frame = ospray::getSysTime() - t0;
    static double nom = 0.f;
    static double den = 0.f;
    den = 0.8f*den + 1.f;
    nom = 0.8f*nom + t_frame;
    std::cout << "done rendering, time per frame = " << (t_frame*1000.f) << "ms, avg'ed fps = " << (den/nom) << std::endl;
#else
    // rendering = true;
    ospray::api::Device::current->renderFrame(fb,renderer,fbChannelFlags);
    // rendering = false;
#endif
  }

  extern "C" void ospCommit(OSPObject object)
  {
    // assert(!rendering);

    ASSERT_DEVICE();
    Assert(object && "invalid object handle to commit to");
    LOG("ospCommit(...)");
    ospray::api::Device::current->commit(object);
  }

  extern "C" void ospSetString(OSPObject _object, const char *id, const char *s)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->setString(_object,id,s);
  }
  extern "C" void ospSetf(OSPObject _object, const char *id, float x)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->setFloat(_object,id,x);
  }
  extern "C" void ospSet1f(OSPObject _object, const char *id, float x)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->setFloat(_object,id,x);
  }
  extern "C" void ospSet1i(OSPObject _object, const char *id, int32 x)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->setInt(_object,id,x);
  }
  extern "C" void ospSeti(OSPObject _object, const char *id, int x)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->setInt(_object,id,x);
  }

  /*! Copy data into the given volume. */
  extern "C" int ospSetRegion(OSPVolume object, void *source, vec3i index, vec3i count) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->setRegion(object, source, index, count));
  }

  /*! add a vec2f parameter to an object */
  extern "C" void ospSetVec2f(OSPObject _object, const char *id, const vec2f &v)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->setVec2f(_object, id, v);
  }
  /*! add a data array to another object */
  extern "C" void ospSetVec3f(OSPObject _object, const char *id, const vec3f &v)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->setVec3f(_object,id,v);
  }
  /*! add a data array to another object */
  extern "C" void ospSetVec3i(OSPObject _object, const char *id, const vec3i &v)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->setVec3i(_object,id,v);
  }
  /*! add a data array to another object */
  extern "C" void ospSet2f(OSPObject _object, const char *id, float x, float y)
  {
    ASSERT_DEVICE();
    ospSetVec2f(_object,id,vec2f(x,y));
  }
  /*! add a data array to another object */
  extern "C" void ospSet2fv(OSPObject _object, const char *id, const float *xy)
  {
    ASSERT_DEVICE();
    ospSetVec2f(_object,id,vec2f(xy[0],xy[1]));
  }
  /*! add a data array to another object */
  extern "C" void ospSet3f(OSPObject _object, const char *id, float x, float y, float z)
  {
    ASSERT_DEVICE();
    ospSetVec3f(_object,id,vec3f(x,y,z));
  }
  /*! add a data array to another object */
  extern "C" void ospSet3fv(OSPObject _object, const char *id, const float *xyz)
  {
    ASSERT_DEVICE();
    ospSetVec3f(_object,id,vec3f(xyz[0],xyz[1],xyz[2]));
  }
  /*! add a data array to another object */
  extern "C" void ospSet3i(OSPObject _object, const char *id, int x, int y, int z)
  {
    ASSERT_DEVICE();
    ospSetVec3i(_object,id,vec3f(x,y,z));
  }
  /*! add a data array to another object */
  extern "C" void ospSetVoidPtr(OSPObject _object, const char *id, void *v)
  {
    ASSERT_DEVICE();
    ospray::api::Device::current->setVoidPtr(_object,id,v);
  }
  extern "C" void ospRelease(OSPObject _object)
  {
    ASSERT_DEVICE();
    if (!_object) return;
    ospray::api::Device::current->release(_object);
  }

  //! assign given material to given geometry
  extern "C" void ospSetMaterial(OSPGeometry geometry, OSPMaterial material)
  {
    ASSERT_DEVICE();
    Assert2(geometry,"NULL geometry passed to ospSetMaterial");
    ospray::api::Device::current->setMaterial(geometry,material);
  }

  //! Get the handle of the named data array associated with an object.
  extern "C" int ospGetData(OSPObject object, const char *name, OSPData *value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getData(object, name, value));
  }

  //! Get a copy of the data in an array (the application is responsible for freeing this pointer).
  extern "C" int ospGetDataValues(OSPData object, void **pointer, size_t *count, OSPDataType *type) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getDataValues(object, pointer, count, type));
  }

  //! Get the named scalar floating point value associated with an object.
  extern "C" int ospGetf(OSPObject object, const char *name, float *value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getf(object, name, value));
  }

  //! Get the named scalar integer associated with an object.
  extern "C" int ospGeti(OSPObject object, const char *name, int *value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->geti(object, name, value));
  }

  //! Get the material associated with a geometry object.
  extern "C" int ospGetMaterial(OSPGeometry geometry, OSPMaterial *value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getMaterial(geometry, value));
  }

  //! Get the named object associated with an object.
  extern "C" int ospGetObject(OSPObject object, const char *name, OSPObject *value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getObject(object, name, value));
  }

  //! Retrieve a NULL-terminated list of the parameter names associated with an object.
  extern "C" int ospGetParameters(OSPObject object, char ***value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getParameters(object, value));
  }

  //! Get a pointer to a copy of the named character string associated with an object.
  extern "C" int ospGetString(OSPObject object, const char *name, char **value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getString(object, name, value));
  }

  //! Get the type of the named parameter or the given object (if 'name' is NULL).
  extern "C" int ospGetType(OSPObject object, const char *name, OSPDataType *value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getType(object, name, value));
  }

  //! Get the named 2-vector floating point value associated with an object.
  extern "C" int ospGetVec2f(OSPObject object, const char *name, osp::vec2f *value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getVec2f(object, name, value));
  }

  //! Get the named 3-vector floating point value associated with an object.
  extern "C" int ospGetVec3f(OSPObject object, const char *name, osp::vec3f *value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getVec3f(object, name, value));
  }

  //! Get the named 3-vector integer value associated with an object.
  extern "C" int ospGetVec3i(OSPObject object, const char *name, osp::vec3i *value) {
    ASSERT_DEVICE();
    return(ospray::api::Device::current->getVec3i(object, name, value));
  }

  /*! \brief create a new instance geometry that instantiates another
    model.  the resulting geometry still has to be added to another
    model via ospAddGeometry */
  extern "C" OSPGeometry ospNewInstance(OSPModel modelToInstantiate,
                                        const osp::affine3f &xfm)
  {
    ASSERT_DEVICE();
    // return ospray::api::Device::current->newInstance(modelToInstantiate,xfm);
    OSPGeometry geom = ospNewGeometry("instance");
    ospSet3fv(geom,"xfm.l.vx",&xfm.l.vx.x);
    ospSet3fv(geom,"xfm.l.vy",&xfm.l.vy.x);
    ospSet3fv(geom,"xfm.l.vz",&xfm.l.vz.x);
    ospSet3fv(geom,"xfm.p",&xfm.p.x);
    ospSetObject(geom,"model",modelToInstantiate);
    return geom;
  }

  extern "C" void ospPick(OSPPickResult *result, OSPRenderer renderer, const vec2f &screenPos)
  {
    ASSERT_DEVICE();
    Assert2(renderer, "NULL renderer passed to ospPick");
    if (!result) return;
    *result = ospray::api::Device::current->pick(renderer, screenPos);
  }

  extern "C" OSPPickData ospUnproject(OSPRenderer renderer, const vec2f &screenPos)
  {
    static bool warned = false;
    if (!warned) {
      std::cout << "'ospUnproject()' has been deprecated. Please use the new function 'ospPick()' instead" << std::endl;
      warned = true;
    }
    ASSERT_DEVICE();
    Assert2(renderer, "NULL renderer passed to ospUnproject");
    vec2f flippedScreenPos = vec2f(screenPos.x, 1.0f - screenPos.y);
    OSPPickResult pick = ospray::api::Device::current->pick(renderer, flippedScreenPos);
    OSPPickData res = { pick.hit,  pick.position.x,  pick.position.y,  pick.position.z };
    return res;
  }

} // ::ospray
