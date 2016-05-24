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

#include "ospray/common/OSPCommon.h"
#include "ospray/include/ospray/ospray.h"
#include "ospray/render/Renderer.h"
#include "ospray/camera/Camera.h"
#include "ospray/common/Material.h"
#include "ospray/volume/Volume.h"
#include "ospray/transferFunction/TransferFunction.h"
#include "LocalDevice.h"
#include "ospray/common/Core.h"

#ifdef _WIN32
#  include <process.h> // for getpid
#endif

#if 1
# define LOG(a) if (ospray::logLevel > 2) { std::cout << "#ospray: " << a << std::endl << std::flush; fflush(0); }
#else
# define LOG(a) /*ignore*/
#endif

/*! \file api.cpp implements the public ospray api functions by
  routing them to a respective \ref device */
namespace ospray {

  using std::endl;
  using std::cout;

#ifdef OSPRAY_MPI
  namespace mpi {
    ospray::api::Device *createMPI_ListenForWorkers(int *ac, const char **av,
                                                    const char *fileNameToStorePortIn);
    ospray::api::Device *createMPI_LaunchWorkerGroup(int *ac, const char **av,
                                                     const char *launchCommand);
    ospray::api::Device *createMPI_runOnExistingRanks(int *ac, const char **av,
                                                      bool ranksBecomeWorkers);
    ospray::api::Device *createMPI_RanksBecomeWorkers(int *ac, const char **av)
    { return createMPI_runOnExistingRanks(ac,av,true); }

    void initDistributedAPI(int *ac, char ***av, OSPDRenderMode mpiMode);
  }
#endif

#ifdef OSPRAY_MIC_COI
  namespace coi {
    ospray::api::Device *createCoiDevice(int *ac, const char **av);
  }
#endif

} // ::ospray

std::string getPidString() {
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
}

#define ASSERT_DEVICE() if (ospray::api::Device::current == NULL)       \
    throw std::runtime_error("OSPRay not yet initialized "              \
                             "(most likely this means you tried to "    \
                             "call an ospray API function before "      \
                             "first calling ospInit())"+getPidString());

using namespace ospray;

extern "C" void ospInit(int *_ac, const char **_av)
{
  if (ospray::api::Device::current) {
    throw std::runtime_error("OSPRay error: device already exists "
                             "(did you call ospInit twice?)");
  }

  auto *nThreads = getenv("OSPRAY_THREADS");
  if (nThreads) {
    numThreads = atoi(nThreads);
  }

  /* call ospray::init to properly parse common args like
     --osp:verbose, --osp:debug etc */
  ospray::init(_ac,&_av);

  const char *OSP_MPI_LAUNCH_FROM_ENV = getenv("OSPRAY_MPI_LAUNCH");

  if (OSP_MPI_LAUNCH_FROM_ENV) {
#ifdef OSPRAY_MPI
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
#ifdef OSPRAY_MPI
        removeArgs(*_ac,(char **&)_av,i,1);
        ospray::api::Device::current
          = mpi::createMPI_RanksBecomeWorkers(_ac,_av);
#else
        throw std::runtime_error("OSPRay MPI support not compiled in");
#endif
        --i;
        continue;
      }

      if (std::string(_av[i]) == "--osp:coi") {
#ifdef __MIC__
        throw std::runtime_error("The COI device can only be created on the host");
#elif defined(OSPRAY_MIC_COI)
        removeArgs(*_ac,(char **&)_av,i,1);
        ospray::api::Device::current
          = ospray::coi::createCoiDevice(_ac,_av);
#else
        throw std::runtime_error("OSPRay's COI support not compiled in");
#endif
        --i;
        continue;
      }

      if (std::string(_av[i]) == "--osp:mpi-launch") {
#ifdef OSPRAY_MPI
        if (i+2 > *_ac)
          throw std::runtime_error("--osp:mpi-launch expects an argument");
        const char *launchCommand = strdup(_av[i+1]);
        removeArgs(*_ac,(char **&)_av,i,2);
        ospray::api::Device::current
          = mpi::createMPI_LaunchWorkerGroup(_ac,_av,launchCommand);
#else
        throw std::runtime_error("OSPRay MPI support not compiled in");
#endif
        --i;
        continue;
      }

      const char *listenArgName = "--osp:mpi-listen";
      if (!strncmp(_av[i],listenArgName,strlen(listenArgName))) {
#ifdef OSPRAY_MPI
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
        --i;
        continue;
      }
    }
  }

  // no device created on cmd line, yet, so default to localdevice
  if (ospray::api::Device::current == NULL) {
    ospray::api::Device::current = new ospray::api::LocalDevice(_ac,_av);
  }
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
                                            const uint32_t channels)
{
  ASSERT_DEVICE();
  return ospray::api::Device::current->frameBufferCreate((vec2i&)size, mode, channels);
}

//! load module \<name\> from shard lib libospray_module_\<name\>.so, or
extern "C" int32_t ospLoadModule(const char *moduleName)
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

extern "C" void ospRemoveGeometry(OSPModel model, OSPGeometry geometry)
{
  ASSERT_DEVICE();
  Assert(model != NULL && "invalid model in ospRemoveGeometry");
  Assert(geometry != NULL && "invalid geometry in ospRemoveGeometry");
  return ospray::api::Device::current->removeGeometry(model, geometry);
}

extern "C" void ospAddVolume(OSPModel model, OSPVolume volume)
{
  ASSERT_DEVICE();
  Assert(model != NULL && "invalid model in ospAddVolume");
  Assert(volume != NULL && "invalid volume in ospAddVolume");
  return ospray::api::Device::current->addVolume(model, volume);
}

extern "C" void ospRemoveVolume(OSPModel model, OSPVolume volume)
{
  ASSERT_DEVICE();
  Assert(model != NULL && "invalid model in ospRemoveVolume");
  Assert(volume != NULL && "invalid volume in ospRemoveVolume");
  return ospray::api::Device::current->removeVolume(model, volume);
}

/*! create a new data buffer, with optional init data and control flags */
extern "C" OSPData ospNewData(size_t nitems, OSPDataType format, const void *init, const uint32_t flags)
{
  ASSERT_DEVICE();
  LOG("ospSetData(...)");
  LOG("ospSetData(...," << nitems << "," << ((int)format) << "," << ((int*)init) << "," << flags << ",...)");
  OSPData data = ospray::api::Device::current->newData(nitems,format,(void*)init,flags);
  LOG("DONE ospSetData(...,\"" << nitems << "," << ((int)format) << "," << ((int*)init) << "," << flags << ",...)");
  return data;
}

/*! add a data array to another object */
extern "C" void ospSetData(OSPObject object, const char *bufName, OSPData data)
{
  // assert(!rendering);
  ASSERT_DEVICE();
  LOG("ospSetData(...,\"" << bufName << "\",...)");
  return ospray::api::Device::current->setObject(object,bufName,(OSPObject)data);
}

/*! add an object parameter to another object */
extern "C" void ospSetParam(OSPObject target, const char *bufName, OSPObject value)
{
  ASSERT_DEVICE();
  static bool warned = false;
  if (!warned) {
    std::cout << "'ospSetParam()' has been deprecated. Please use the new naming convention of 'ospSetObject()' instead" << std::endl;
    warned = true;
  }
  LOG("ospSetParam(...,\"" << bufName << "\",...)");
  return ospray::api::Device::current->setObject(target,bufName,value);
}

/*! set/add a pixel op to a frame buffer */
extern "C" void ospSetPixelOp(OSPFrameBuffer fb, OSPPixelOp op)
{
  ASSERT_DEVICE();
  LOG("ospSetPixelOp(...,...)");
  return ospray::api::Device::current->setPixelOp(fb,op);
}

/*! add an object parameter to another object */
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
  char *type = (char *)alloca(L+1);
  for (int i=0;i<=L;i++) {
    char c = _type[i];
    if (c == '-' || c == ':')
      c = '_';
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

  std::string type(_type);
  for (size_t i = 0; i < type.size(); i++) {
    if (type[i] == '-' || type[i] == ':')
      type[i] = '_';
  }
  OSPRenderer renderer = ospray::api::Device::current->newRenderer(type.c_str());
  if ((ospray::logLevel > 0) && (renderer == NULL)) {
    std::cerr << "#ospray: could not create renderer '" << type << "'" << std::endl;
  }
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
  if ((ospray::logLevel > 0) && (geometry == NULL))
    std::cerr << "#ospray: could not create geometry '" << type << "'" << std::endl;
  LOG("DONE ospNewGeometry(" << type << ") >> " << (int *)geometry);
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
  if ((ospray::logLevel > 0) && (material == NULL))
    std::cerr << "#ospray: could not create material '" << type << "'" << std::endl;
  return material;
}

extern "C" OSPLight ospNewLight(OSPRenderer renderer, const char *type)
{
  ASSERT_DEVICE();
  Assert2(type != NULL, "invalid light type identifier in ospNewLight");
  LOG("ospNewLight(" << renderer << ", " << type << ")");
  OSPLight light = ospray::api::Device::current->newLight(renderer, type);
  if ((ospray::logLevel > 0) && (light == NULL))
    std::cerr << "#ospray: could not create light '" << type << "'" << std::endl;
  return light;
}

/*! \brief create a new camera of given type

  return 'NULL' if that type is not known */
extern "C" OSPCamera ospNewCamera(const char *type)
{
  ASSERT_DEVICE();
  Assert(type != NULL && "invalid camera type identifier in ospNewCamera");
  LOG("ospNewCamera(" << type << ")");
  OSPCamera camera = ospray::api::Device::current->newCamera(type);
  if ((ospray::logLevel > 0) && (camera == NULL))
    std::cerr << "#ospray: could not create camera '" << type << "'" << std::endl;
  return camera;
}

extern "C" OSPTexture2D ospNewTexture2D(const osp::vec2i &size,
                                        const OSPTextureFormat type,
                                        void *data,
                                        const uint32_t flags)
{
  ASSERT_DEVICE();
  Assert2(size.x > 0, "Width must be greater than 0 in ospNewTexture2D");
  Assert2(size.y > 0, "Height must be greater than 0 in ospNewTexture2D");
  LOG("ospNewTexture2D( (" << size.x << ", " << size.y << "), " << type << ", " << data << ", " << flags << ")");
  return ospray::api::Device::current->newTexture2D((vec2i&)size, type, data, flags);
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
      cout << "ospNewVolume: " << type << endl;
      // cout << "ospNewVolume: " << ((ospray::Volume*)volume)->toString() << endl;
    else
      std::cerr << "#ospray: could not create volume '" << type << "'" << std::endl;
  }
  if ((ospray::logLevel > 0) && (volume == NULL))
    std::cerr << "#ospray: could not create volume '" << type << "'" << std::endl;
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
      cout << "ospNewTransferFunction(" << type << ")" << endl;
    else
      std::cerr << "#ospray: could not create transfer function '" << type << "'" << std::endl;
  }
  if ((ospray::logLevel > 0) && (transferFunction == NULL))
    std::cerr << "#ospray: could not create transferFunction '" << type << "'" << std::endl;
  return transferFunction;
}

extern "C" void ospFrameBufferClear(OSPFrameBuffer fb,
                                    const uint32_t fbChannelFlags)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->frameBufferClear(fb,fbChannelFlags);
}

/*! \brief call a renderer to render given model into given framebuffer

  model _may_ be empty (though most framebuffers will expect one!) */
extern "C" float ospRenderFrame(OSPFrameBuffer fb,
                               OSPRenderer renderer,
                               const uint32_t fbChannelFlags
                               )
{
  ASSERT_DEVICE();
#if 0
  double t0 = ospray::getSysTime();
  ospray::api::Device::current->renderFrame(fb,renderer,fbChannelFlags);
  double t_frame = ospray::getSysTime() - t0;
  static double nom = 0.f;
  static double den = 0.f;
  den = 0.95f*den + 1.f;
  nom = 0.95f*nom + t_frame;
  std::cout << "done rendering, time per frame = " << (t_frame*1000.f) << "ms, avg'ed fps = " << (den/nom) << std::endl;
#else
  // rendering = true;
  return ospray::api::Device::current->renderFrame(fb,renderer,fbChannelFlags);
  // rendering = false;
#endif
}

extern "C" void ospCommit(OSPObject object)
{
  // assert(!rendering);
  LOG("ospCommit(...)");
  ASSERT_DEVICE();
  Assert(object && "invalid object handle to commit to");
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
extern "C" void ospSet1i(OSPObject _object, const char *id, int32_t x)
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
extern "C" int ospSetRegion(OSPVolume object, void *source,
                            const osp::vec3i &index, const osp::vec3i &count)
{
  ASSERT_DEVICE();
  return(ospray::api::Device::current->setRegion(object, source, 
                                                 (vec3i&)index,
                                                 (vec3i&)count));
}

/*! add a vec2f parameter to an object */
extern "C" void ospSetVec2f(OSPObject _object, const char *id, const osp::vec2f &v)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec2f(_object, id, (const vec2f &)v);
}

/*! add a vec2i parameter to an object */
extern "C" void ospSetVec2i(OSPObject _object, const char *id, const osp::vec2i &v)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec2i(_object, id, (const vec2i &)v);
}

/*! add a vec3f parameter to another object */
extern "C" void ospSetVec3f(OSPObject _object, const char *id, const osp::vec3f &v)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec3f(_object,id,(const vec3f &)v);
}

/*! add a vec4f parameter to another object */
extern "C" void ospSetVec4f(OSPObject _object, const char *id, const osp::vec4f &v)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec4f(_object,id,(const vec4f &)v);
}

/*! add a vec3i parameter to another object */
extern "C" void ospSetVec3i(OSPObject _object, const char *id, const osp::vec3i &v)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec3i(_object,id,(const vec3i &)v);
}

/*! add a vec2f parameter to another object */
extern "C" void ospSet2f(OSPObject _object, const char *id, float x, float y)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec2f(_object,id,ospray::vec2f(x,y));
}

/*! add a vec2f parameter to another object */
extern "C" void ospSet2fv(OSPObject _object, const char *id, const float *xy)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec2f(_object,id,vec2f(xy[0],xy[1]));
}

/*! add a vec2i parameter to another object */
extern "C" void ospSet2i(OSPObject _object, const char *id, int x, int y)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec2i(_object,id,vec2i(x,y));
}

/*! add a vec2i parameter to another object */
extern "C" void ospSet2iv(OSPObject _object, const char *id, const int *xy)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec2i(_object,id,vec2i(xy[0],xy[1]));
}

/*! add a vec3f parameter to another object */
extern "C" void ospSet3f(OSPObject _object, const char *id, float x, float y, float z)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec3f(_object,id,vec3f(x,y,z));
}

/*! add a vec3f parameter to another object */
extern "C" void ospSet3fv(OSPObject _object, const char *id, const float *xyz)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec3f(_object,id,vec3f(xyz[0],xyz[1],xyz[2]));
}

/*! add a vec3i parameter to another object */
extern "C" void ospSet3i(OSPObject _object, const char *id, int x, int y, int z)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec3i(_object,id,vec3i(x,y,z));
}

/*! add a vec3i parameter to another object */
extern "C" void ospSet3iv(OSPObject _object, const char *id, const int *xyz)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec3i(_object,id,vec3i(xyz[0],xyz[1],xyz[2]));
}

/*! add a vec4f parameter to another object */
extern "C" void ospSet4f(OSPObject _object, const char *id, float x, float y, float z, float w)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec4f(_object,id,vec4f(x,y,z,w));
}

/*! add a vec4f parameter to another object */
extern "C" void ospSet4fv(OSPObject _object, const char *id, const float *xyzw)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->setVec4f(_object,id,vec4f(xyzw[0],xyzw[1],xyzw[2],xyzw[3]));
}

/*! add a void pointer to another object */
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
  ospSetObject(geom,"model",modelToInstantiate);
  return geom;
}

extern "C" void ospPick(OSPPickResult *result,
                        OSPRenderer renderer,
                        const osp::vec2f &screenPos)
{
  ASSERT_DEVICE();
  Assert2(renderer, "NULL renderer passed to ospPick");
  if (!result) return;
  *result = ospray::api::Device::current->pick(renderer,
                                               (const vec2f&)screenPos);
}

//! \brief allows for switching the MPI scope from "per rank" to "all ranks"
// extern "C" void ospdApiMode(OSPDApiMode mode)
// {
//   ASSERT_DEVICE();
//   ospray::api::Device::current->apiMode(mode);
// }

#ifdef OSPRAY_MPI
//! \brief initialize the ospray engine (for use with MPI-parallel app)
/*! \detailed Note the application must call this function "INSTEAD OF"
  MPI_Init(), NOT "in addition to" */
extern "C" void ospdMpiInit(int *ac, char ***av, OSPDRenderMode mode)
{
  if (ospray::api::Device::current != NULL)
    throw std::runtime_error("#osp:mpi: OSPRay already initialized!?");
  ospray::mpi::initDistributedAPI(ac,av,mode);
}

//! the 'lid to the pot' of ospdMpiInit().
/*! does both an osp shutdown and an mpi shutdown for the mpi group
  created with ospdMpiInit */
extern "C" void ospdMpiShutdown()
{
}
#endif

extern "C" void ospSampleVolume(float **results,
                                OSPVolume volume,
                                const osp::vec3f &worldCoordinates,
                                const size_t count)
{
  ASSERT_DEVICE();
  Assert2(volume, "NULL volume passed to ospSampleVolume");

  if (count == 0) {
    *results = NULL;
    return;
  }

  ospray::api::Device::current->sampleVolume(results, volume,
                                             (vec3f*)&worldCoordinates, count);
}

