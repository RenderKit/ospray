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

#include "common/OSPCommon.h"
#include "common/Util.h"
#include "include/ospray/ospray.h"
#include "render/Renderer.h"
#include "camera/Camera.h"
#include "common/Material.h"
#include "volume/Volume.h"
#include "transferFunction/TransferFunction.h"
#include "LocalDevice.h"

#ifdef _WIN32
#  include <process.h> // for getpid
#endif

# define LOG(a) if (ospray::logLevel() > 2) { std::cout << "#ospray: " << a << std::endl; }

/*! \file api.cpp implements the public ospray api functions by
  routing them to a respective \ref device */

std::string getPidString() {
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
}

#define ASSERT_DEVICE() if (!ospray::api::Device::current)              \
    throw std::runtime_error("OSPRay not yet initialized "              \
                             "(most likely this means you tried to "    \
                             "call an ospray API function before "      \
                             "first calling ospInit())"+getPidString());

using namespace ospray;

inline ospray::api::Device *createMpiDevice()
{
  ospray::api::Device *device = nullptr;

  try {
    device = ospray::api::Device::createDevice("mpi");
  } catch (const std::runtime_error &) {
    try {
      ospLoadModule("mpi");
      device = ospray::api::Device::createDevice("mpi");
    } catch (const std::runtime_error &err) {
      std::string error_msg = "Cannot create a device of type 'mpi'! Make sure "
                              "you have enabled the OSPRAY_MODULE_MPI CMake "
                              "variable in your build of OSPRay.";
      error_msg += ("\n(Reason device creation failed: "+std::string(err.what())+")");
      throw std::runtime_error(error_msg);
    }
  }

  return device;
}

extern "C" void ospInit(int *_ac, const char **_av)
{
  if (ospray::api::Device::current) {
    throw std::runtime_error("OSPRay error: device already exists "
                             "(did you call ospInit twice?)");
  }

  auto OSP_MPI_LAUNCH = getEnvVar<std::string>("OSPRAY_MPI_LAUNCH");

  if (OSP_MPI_LAUNCH.first) {
    postErrorMsg("#osp: launching ospray mpi ring - "
                 "make sure that mpd is running");

    auto *mpiDevice = createMpiDevice();
    ospray::api::Device::current = mpiDevice;
    mpiDevice->findParam("mpiMode", true)->set("mpi-launch");
    mpiDevice->findParam("launchCommand", true)
             ->set(OSP_MPI_LAUNCH.second.c_str());
  }
  if (_ac && _av) {
    for (int i = 1; i < *_ac; i++) {
      std::string av(_av[i]);

      if (av == "--osp:coi") {
        throw std::runtime_error("OSPRay's COI device is no longer supported!");
        --i;
        continue;
      }

      auto deviceSwitch = av.substr(0, 13);
      if (deviceSwitch == "--osp:device:") {
        removeArgs(*_ac,(char **&)_av,i,1);
        auto deviceName = av.substr(13);
        
        try {
          auto *device = ospray::api::Device::createDevice(deviceName.c_str());
          ospray::api::Device::current = device;
        } catch (const std::runtime_error &) {
          throw std::runtime_error("Failed to create device of type '"
                                   + deviceName + "'! Perhaps you spelled the "
                                   "device name wrong or didn't link the "
                                   "library which defines the device?");
        }
        --i;
        continue;
      }

      if (av == "--osp:mpi") {
        removeArgs(*_ac,(char **&)_av,i,1);
        auto *mpiDevice = createMpiDevice();
        ospray::api::Device::current = mpiDevice;
        --i;
        continue;
      }

      if (av == "--osp:mpi-launch") {
        if (i+2 > *_ac)
          throw std::runtime_error("--osp:mpi-launch expects an argument");
        const char *launchCommand = strdup(_av[i+1]);
        removeArgs(*_ac,(char **&)_av,i,2);

        auto *mpiDevice = createMpiDevice();
        ospray::api::Device::current = mpiDevice;
        mpiDevice->findParam("mpiMode", true)->set("mpi-launch");
        mpiDevice->findParam("launchCommand", true)->set(launchCommand);
        --i;
        continue;
      }

      const char *listenArgName = "--osp:mpi-listen";
      if (!strncmp(_av[i], listenArgName, strlen(listenArgName))) {
        const char *fileNameToStorePortIn = nullptr;
        if (strlen(_av[i]) > strlen(listenArgName)) {
          fileNameToStorePortIn = strdup(_av[i]+strlen(listenArgName)+1);
        }
        removeArgs(*_ac,(char **&)_av,i,1);

        auto *mpiDevice = createMpiDevice();
        ospray::api::Device::current = mpiDevice;
        mpiDevice->findParam("mpiMode", true)->set("mpi-listen");
        mpiDevice->findParam("fileNameToStorePortIn", true)
                 ->set(fileNameToStorePortIn);
        --i;
        continue;
      }
    }
  }

  // no device created on cmd line, yet, so default to localdevice
  if (!ospray::api::Device::current) {
    ospray::api::Device::current = new ospray::api::LocalDevice;
  }

  ospray::initFromCommandLine(_ac,&_av);

  ospray::api::Device::current->commit();
}

extern "C" OSPDevice ospCreateDevice(const char *deviceType)
{
  return (OSPDevice)ospray::api::Device::createDevice(deviceType);
}

extern "C" void ospSetCurrentDevice(OSPDevice _device)
{
  auto *device = (ospray::api::Device*)_device;

  if (!device->isCommitted()) {
    throw std::runtime_error("You must commit the device before using it!");
  }

  ospray::api::Device::current = device;
}

extern "C" OSPDevice ospGetCurrentDevice()
{
  return (OSPDevice)ospray::api::Device::current.ptr;
}


/*! destroy a given frame buffer.

  due to internal reference counting the framebuffer may or may not be deleted immediately
*/
extern "C" void ospFreeFrameBuffer(OSPFrameBuffer fb)
{
  ASSERT_DEVICE();
  Assert(fb != nullptr);
  ospray::api::Device::current->release(fb);
}

extern "C" OSPFrameBuffer ospNewFrameBuffer(const osp::vec2i &size,
                                            const OSPFrameBufferFormat mode,
                                            const uint32_t channels)
{
  ASSERT_DEVICE();
  return ospray::api::Device::current->frameBufferCreate((vec2i&)size, mode, channels);
}

//! load module \<name\> from shard lib libospray_module_\<name\>.so
extern "C" int32_t ospLoadModule(const char *moduleName)
{
  if (ospray::api::Device::current) {
    return ospray::api::Device::current->loadModule(moduleName);
  } else {
    return loadLocalModule(moduleName);
  }
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
  Assert(mapped != nullptr && "invalid mapped pointer in ospUnmapFrameBuffer");
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
  Assert(model != nullptr && "invalid model in ospAddGeometry");
  Assert(geometry != nullptr && "invalid geometry in ospAddGeometry");
  return ospray::api::Device::current->addGeometry(model,geometry);
}

extern "C" void ospRemoveGeometry(OSPModel model, OSPGeometry geometry)
{
  ASSERT_DEVICE();
  Assert(model != nullptr && "invalid model in ospRemoveGeometry");
  Assert(geometry != nullptr && "invalid geometry in ospRemoveGeometry");
  return ospray::api::Device::current->removeGeometry(model, geometry);
}

extern "C" void ospAddVolume(OSPModel model, OSPVolume volume)
{
  ASSERT_DEVICE();
  Assert(model != nullptr && "invalid model in ospAddVolume");
  Assert(volume != nullptr && "invalid volume in ospAddVolume");
  return ospray::api::Device::current->addVolume(model, volume);
}

extern "C" void ospRemoveVolume(OSPModel model, OSPVolume volume)
{
  ASSERT_DEVICE();
  Assert(model != nullptr && "invalid model in ospRemoveVolume");
  Assert(volume != nullptr && "invalid volume in ospRemoveVolume");
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
  ASSERT_DEVICE();
  LOG("ospSetData(...,\"" << bufName << "\",...)");
  return ospray::api::Device::current->setObject(object,bufName,(OSPObject)data);
}

/*! add an object parameter to another object */
extern "C" void ospSetParam(OSPObject target, const char *bufName, OSPObject value)
{
  ASSERT_DEVICE();
  static WarnOnce warning("'ospSetParam()' has been deprecated. "
                          "Please use the new naming convention of "
                          "'ospSetObject()' instead");
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

  return 'nullptr' if that type is not known */
extern "C" OSPPixelOp ospNewPixelOp(const char *_type)
{
  ASSERT_DEVICE();
  Assert2(_type,"invalid render type identifier in ospNewPixelOp");
  LOG("ospNewPixelOp(" << _type << ")");
  int L = strlen(_type);
  char *type = STACK_BUFFER(char, L+1);
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

  return 'nullptr' if that type is not known */
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
  if (renderer == nullptr) {
    std::stringstream msg;
    msg << "#ospray: could not create renderer '" << type << "'" << std::endl;
    postErrorMsg(msg, 1);
  }
  return renderer;
}

/*! \brief create a new geometry of given type

  return 'nullptr' if that type is not known */
extern "C" OSPGeometry ospNewGeometry(const char *type)
{
  ASSERT_DEVICE();
  Assert(type != nullptr && "invalid geometry type identifier in ospNewGeometry");
  LOG("ospNewGeometry(" << type << ")");
  OSPGeometry geometry = ospray::api::Device::current->newGeometry(type);
  if (geometry == nullptr) {
    std::stringstream msg;
    msg << "#ospray: could not create geometry '" << type << "'" << std::endl;
    postErrorMsg(msg, 1);
  }
  LOG("DONE ospNewGeometry(" << type << ") >> " << (int *)geometry);
  return geometry;
}

/*! \brief create a new material of given type

  return 'nullptr' if that type is not known */
extern "C" OSPMaterial ospNewMaterial(OSPRenderer renderer, const char *type)
{
  ASSERT_DEVICE();
  // Assert2(renderer != nullptr, "invalid renderer handle in ospNewMaterial");
  Assert2(type != nullptr, "invalid material type identifier in ospNewMaterial");
  LOG("ospNewMaterial(" << renderer << ", " << type << ")");
  OSPMaterial material = ospray::api::Device::current->newMaterial(renderer, type);
  if (material == nullptr) {
    std::stringstream msg;
    msg << "#ospray: could not create material '" << type << "'" << std::endl;
    postErrorMsg(msg, 1);
  }
  return material;
}

extern "C" OSPLight ospNewLight(OSPRenderer renderer, const char *type)
{
  ASSERT_DEVICE();
  Assert2(type != nullptr, "invalid light type identifier in ospNewLight");
  LOG("ospNewLight(" << renderer << ", " << type << ")");
  OSPLight light = ospray::api::Device::current->newLight(renderer, type);
  if (light == nullptr) {
    std::stringstream msg;
    msg << "#ospray: could not create light '" << type << "'" << std::endl;
    postErrorMsg(msg, 1);
  }
  return light;
}

/*! \brief create a new camera of given type

  return 'nullptr' if that type is not known */
extern "C" OSPCamera ospNewCamera(const char *type)
{
  ASSERT_DEVICE();
  Assert(type != nullptr && "invalid camera type identifier in ospNewCamera");
  LOG("ospNewCamera(" << type << ")");
  OSPCamera camera = ospray::api::Device::current->newCamera(type);
  if (camera == nullptr) {
    std::stringstream msg;
    msg << "#ospray: could not create camera '" << type << "'" << std::endl;
    postErrorMsg(msg, 1);
  }
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

/*! \brief create a new volume of given type, return 'nullptr' if that type is not known */
extern "C" OSPVolume ospNewVolume(const char *type)
{
  ASSERT_DEVICE();
  Assert(type != nullptr && "invalid volume type identifier in ospNewVolume");
  LOG("ospNewVolume(" << type << ")");
  OSPVolume volume = ospray::api::Device::current->newVolume(type);
  if (volume == nullptr) {
    std::stringstream msg;
    msg << "#ospray: could not create volume '" << type << "'" << std::endl;
    postErrorMsg(msg, 1);
  }
  return volume;
}

/*! \brief create a new transfer function of given type
  return 'nullptr' if that type is not known */
extern "C" OSPTransferFunction ospNewTransferFunction(const char *type)
{
  ASSERT_DEVICE();
  Assert(type != nullptr && "invalid transfer function type identifier in ospNewTransferFunction");
  LOG("ospNewTransferFunction(" << type << ")");
  OSPTransferFunction transferFunction = ospray::api::Device::current->newTransferFunction(type);
  if (transferFunction == nullptr) {
    std::stringstream msg;
    msg << "#ospray: could not create transferFunction '" << type << "'" << std::endl;
    postErrorMsg(msg, 1);
  }
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
  return ospray::api::Device::current->renderFrame(fb,renderer,fbChannelFlags);
}

extern "C" void ospCommit(OSPObject object)
{
  LOG("ospCommit(...)");
  ASSERT_DEVICE();
  Assert(object && "invalid object handle to commit to");
  ospray::api::Device::current->commit(object);
}

extern "C" void ospDeviceCommit(OSPDevice _object)
{
  auto *object = (ospray::api::Device *)_object;
  object->commit();
}

extern "C" void ospDeviceSetString(OSPDevice _object,
                                   const char *id,
                                   const char *s)
{
  ManagedObject *object = (ManagedObject *)_object;
  object->findParam(id, true)->set(s);
}

extern "C" void ospDeviceSet1i(OSPDevice _object, const char *id, int32_t x)
{
  ManagedObject *object = (ManagedObject *)_object;
  object->findParam(id, true)->set(x);
}

extern "C" void ospDeviceSetErrorMsgFunc(OSPDevice object,
                                         OSPErrorMsgFunc callback)
{
  auto *device = (ospray::api::Device *)object;
  device->error_fcn = callback;
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

extern "C" void ospRemoveParam(OSPObject _object, const char *id)
{
  ASSERT_DEVICE();
  ospray::api::Device::current->removeParam(_object, id);
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
  Assert2(geometry,"nullptr geometry passed to ospSetMaterial");
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
  Assert2(renderer, "nullptr renderer passed to ospPick");
  if (!result) return;
  *result = ospray::api::Device::current->pick(renderer,
                                               (const vec2f&)screenPos);
}

extern "C" void ospSampleVolume(float **results,
                                OSPVolume volume,
                                const osp::vec3f &worldCoordinates,
                                const size_t count)
{
  ASSERT_DEVICE();
  Assert2(volume, "nullptr volume passed to ospSampleVolume");

  if (count == 0) {
    *results = nullptr;
    return;
  }

  ospray::api::Device::current->sampleVolume(results, volume,
                                             (vec3f*)&worldCoordinates, count);
}

