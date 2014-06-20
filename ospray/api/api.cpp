#include "ospray/include/ospray/ospray.h"
#include "ospray/render/renderer.h"
#include "ospray/camera/camera.h"
#include "ospray/common/material.h"
#include "ospray/volume/volume.h"
#include "ospray/transferfunction/TransferFunction.h"
#include "localdevice.h"
#include "ospray/common/ospcommon.h"

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
        }

        if (std::string(_av[i]) == "--osp:mpi-launch") {
#if OSPRAY_MPI
          if (i+2 >= *_ac)
            throw std::runtime_error("--osp:mpi-launch expects an argument");
          const char *launchCommand = strdup(_av[i+1]);
          removeArgs(*_ac,(char **&)_av,i,2);
          ospray::api::Device::current
            = mpi::createMPI_LaunchWorkerGroup(_ac,_av,launchCommand);
#else
          throw std::runtime_error("OSPRay MPI support not compiled in");
#endif
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
        }
      }
    }
    
    // no device created on cmd line, yet, so default to localdevice
    if (ospray::api::Device::current == NULL)
      ospray::api::Device::current = new ospray::api::LocalDevice(_ac,_av);
  }

  /*! destroy a given frame buffer. 

    due to internal reference counting the framebuffer may or may not be deleted immeidately
  */
  extern "C" void ospFreeFrameBuffer(OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(fb != NULL);
    std::cout << "warning: ospFreeFrameBuffer not yet implemented - ignoring (this means there is a memory hole!)" << std::endl;
  }

  extern "C" OSPFrameBuffer ospNewFrameBuffer(const osp::vec2i &size, 
                                              const OSPFrameBufferMode mode)
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->frameBufferCreate(size,mode);
  }

  //! load module \<name\> from shard lib libospray_module_\<name\>.so, or 
  extern "C" error_t ospLoadModule(const char *moduleName)
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->loadModule(moduleName);
  }

  extern "C" const void *ospMapFrameBuffer(OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    return ospray::api::Device::current->frameBufferMap(fb);
  }
  
  extern "C" void ospUnmapFrameBuffer(const void *mapped,
                                      OSPFrameBuffer fb)
  {
    ASSERT_DEVICE();
    Assert(mapped != NULL && "invalid mapped pointer in ospAddGeometry");
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
    ASSERT_DEVICE();
    LOG("ospSetData(...,\"" << bufName << "\",...)");
    return ospray::api::Device::current->setObject(object,bufName,(OSPObject)data);
  }

  /*! add a data array to another object */
  extern "C" void ospSetParam(OSPObject target, const char *bufName, OSPObject value)
  {
    ASSERT_DEVICE();
    LOG("ospSetData(...,\"" << bufName << "\",...)");
    return ospray::api::Device::current->setObject(target,bufName,value);
  }

  /*! \brief create a new renderer of given type 

    return 'NULL' if that type is not known */
  extern "C" OSPRenderer ospNewRenderer(const char *_type)
  {
    ASSERT_DEVICE();
    Assert2(_type,"invalid render type identifier in ospAddGeometry");
    LOG("ospNewRenderer(" << _type << ")");
    int L = strlen(_type);
    char type[L+1];
    for (int i=0;i<=L;i++) {
      char c = _type[i];
      if (c == '-' || c == ':') c = '_';
      type[i] = c;
    }
    OSPRenderer renderer = ospray::api::Device::current->newRenderer(type);
    // cant typecast on MPI device!
    // if (ospray::logLevel > 0)
    //   if (renderer) 
    //     cout << "ospNewRenderer: " << ((ospray::Renderer*)renderer)->toString() << endl;
    //   else
    //     std::cerr << "#ospray: could not create renderer '" << type << "'" << std::endl;
    return renderer;
  }
  
  /*! \brief create a new geometry of given type 

    return 'NULL' if that type is not known */
  extern "C" OSPGeometry ospNewGeometry(const char *type)
  {
    ASSERT_DEVICE();
    Assert(type != NULL && "invalid render type identifier in ospAddGeometry");
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
    return ospray::api::Device::current->newLight(renderer, type);
  }

  /*! \brief create a new camera of given type 

    return 'NULL' if that type is not known */
  extern "C" OSPCamera ospNewCamera(const char *type)
  {
    ASSERT_DEVICE();
    Assert(type != NULL && "invalid render type identifier in ospAddGeometry");
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
  /*! \brief create a new volume of given type 
    
    return 'NULL' if that type is not known */
  extern "C" OSPVolume ospNewVolume(const char *type)
  {
    ASSERT_DEVICE();
    Assert(type != NULL && "invalid render type identifier in ospAddGeometry");
    LOG("ospNewVolume(" << type << ")");
    OSPVolume volume = ospray::api::Device::current->newVolume(type);
    if (ospray::logLevel > 0)
      if (volume) 
        cout << "ospNewVolume: " << ((ospray::Volume*)volume)->toString() << endl;
      else
        std::cerr << "#ospray: could not create volume '" << type << "'" << std::endl;
    return volume;
  }

  /*! \brief create a new volume of the given type from data in the given file
    return 'NULL' if the type is not known or the file cannot be found */
  extern "C" OSPVolume ospNewVolumeFromFile(const char *filename, const char *type)
  {
    ASSERT_DEVICE();
    Assert(type != NULL && "invalid volume type identifier in ospNewVolumeFromFile");
    Assert(filename != NULL && "no file name specified in ospNewVolumeFromFile");
    LOG("ospNewVolumeFromFile(" << filename << ", " << type << ")");
    OSPVolume volume = ospray::api::Device::current->newVolumeFromFile(filename, type);
    if (ospray::logLevel > 0)
      if (volume)
        cout << "ospNewVolumeFromFile: " << ((ospray::Volume *) volume)->toString() << endl;
      else
        std::cerr << "#ospray: could not create volume of type '" << type << "' from file '" << filename << "'" << std::endl;
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
    if(ospray::logLevel > 0)
      if(transferFunction)
        cout << "ospNewTransferFunction: " << ((ospray::TransferFunction*)transferFunction)->toString() << endl;
      else
        std::cerr << "#ospray: could not create transfer function '" << type << "'" << std::endl;
    return transferFunction;
  }

  /*! \brief call a renderer to render given model into given framebuffer 
    
    model _may_ be empty (though most framebuffers will expect one!) */
  extern "C" void ospRenderFrame(OSPFrameBuffer fb, OSPRenderer renderer)
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
    ospray::api::Device::current->renderFrame(fb,renderer);
#endif
  }

  /*! add a data array to another object */
  extern "C" void ospCommit(OSPObject object)
  {
    ASSERT_DEVICE();
    Assert(object && "invalid object handle to commit to");
    LOG("ospCommit(...)");
    return ospray::api::Device::current->commit(object);
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
    ospSetParam(geom,"model",modelToInstantiate);
    return geom;
  }

}
