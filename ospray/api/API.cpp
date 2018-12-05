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

//ospcommon
#include "ospcommon/utility/OnScopeExit.h"
#include "ospcommon/utility/getEnvVar.h"
#include "ospcommon/library.h"

//ospray
#include "common/OSPCommon.h"
#include "include/ospray/ospray.h"
#include "Device.h"

#ifdef _WIN32
#  include <process.h> // for getpid
#endif

using ospray::api::Device;
using ospray::api::deviceIsSet;
using ospray::api::currentDevice;

/*! \file api.cpp implements the public ospray api functions by
  routing them to a respective \ref device */

inline std::string getPidString()
{
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
}

#define ASSERT_DEVICE() if (!deviceIsSet())                                   \
    throw std::runtime_error("OSPRay not yet initialized "                    \
                             "(most likely this means you tried to "          \
                             "call an ospray API function before "            \
                             "first calling ospInit())" + getPidString());

#define OSPRAY_CATCH_BEGIN try {                                              \
  auto *fcn_name_ = __PRETTY_FUNCTION__;                                      \
  ospcommon::utility::OnScopeExit guard([&]() {                               \
    postTraceMsg(fcn_name_);                                                  \
  });

#define OSPRAY_CATCH_END(a)                                                   \
  } catch (const std::bad_alloc &) {                                          \
    handleError(OSP_OUT_OF_MEMORY, "OSPRay was unable to allocate memory");   \
    return a;                                                                 \
  } catch (const std::exception &e) {                                         \
    handleError(OSP_UNKNOWN_ERROR, e.what());                                 \
    return a;                                                                 \
  } catch (...) {                                                             \
    handleError(OSP_UNKNOWN_ERROR, "an unrecognized exception was caught");   \
    return a;                                                                 \
  }

using namespace ospray;

inline Device *createMpiDevice(const std::string &type)
{
  Device *device = nullptr;

  try {
    device = Device::createDevice(type.c_str());
  } catch (const std::runtime_error &) {
    try {
      ospLoadModule("mpi");
      device = Device::createDevice(type.c_str());
    } catch (const std::runtime_error &err) {
      std::stringstream error_msg;
      error_msg << "Cannot create a device of type '" << type << "'! Make sure "
                << "you have enabled the OSPRAY_MODULE_MPI CMake "
                << "variable in your build of OSPRay.\n";
      error_msg << "(Reason device creation failed: " << err.what() << ')';
      throw std::runtime_error(error_msg.str());
    }
  }

  return device;
}

extern "C" OSPError ospInit(int *_ac, const char **_av)
OSPRAY_CATCH_BEGIN
{
  auto &currentDevice = Device::current;

  if (currentDevice) {
    throw std::runtime_error("device already exists [ospInit() called twice?]");
  }

  auto OSP_MPI_LAUNCH = utility::getEnvVar<std::string>("OSPRAY_MPI_LAUNCH");

  if (OSP_MPI_LAUNCH) {
    postStatusMsg("#osp: launching ospray mpi ring - "
                 "make sure that mpd is running");

    currentDevice.reset(createMpiDevice("mpi_offload"));
    currentDevice->setParam<std::string>("mpiMode", "mpi-launch");
    currentDevice->setParam<std::string>("launchCommand",
                                         OSP_MPI_LAUNCH.value());
  }

  if (_ac && _av) {
    for (int i = 1; i < *_ac; i++) {
      std::string av(_av[i]);

      if (av == "--osp:coi") {
        handleError(OSP_INVALID_ARGUMENT,
                    "OSPRay's COI device is no longer supported!");
        return OSP_INVALID_ARGUMENT;
      }

      if (av == "--osp:stream") {
        removeArgs(*_ac,_av,i,1);
        loadLocalModule("stream");
        currentDevice.reset(Device::createDevice("stream"));

        --i;
        continue;
      }

      auto moduleSwitch = av.substr(0, 13);
      if (moduleSwitch == "--osp:module:") {
        removeArgs(*_ac,_av,i,1);

        auto moduleName = av.substr(13);
        loadLocalModule(moduleName);

        --i;
        continue;
      }

      auto deviceSwitch = av.substr(0, 13);
      if (deviceSwitch == "--osp:device:") {
        removeArgs(*_ac,_av,i,1);
        auto deviceName = av.substr(13);

        try {
          currentDevice.reset(Device::createDevice(deviceName.c_str()));
        } catch (const std::runtime_error &) {
          throw std::runtime_error("Failed to create device of type '"
                                   + deviceName + "'! Perhaps you spelled the "
                                   "device name wrong or didn't link the "
                                   "library which defines the device?");
        }
        --i;
        continue;
      }

      if (av == "--osp:mpi" || av == "--osp:mpi-offload") {
        removeArgs(*_ac,_av,i,1);
        if (!currentDevice)
          currentDevice.reset(createMpiDevice("mpi_offload"));
        --i;
        continue;
      }

      if (av == "--osp:mpi-distributed") {
        removeArgs(*_ac,_av,i,1);
        if (!currentDevice)
          currentDevice.reset(createMpiDevice("mpi_distributed"));
        --i;
        continue;
      }

      if (av == "--osp:mpi-launch") {
        if (i+2 > *_ac)
          throw std::runtime_error("--osp:mpi-launch expects an argument");
        currentDevice.reset(createMpiDevice("mpi_offload"));
        currentDevice->setParam<std::string>("mpiMode", "mpi-launch");
        currentDevice->setParam<std::string>("launchCommand", _av[i+1]);
        removeArgs(*_ac,_av,i,2);
        --i;
        continue;
      }

      const char *listenArgName = "--osp:mpi-listen";
      if (!strncmp(_av[i], listenArgName, strlen(listenArgName))) {
        const char *fileNameToStorePortIn = nullptr;
        if (strlen(_av[i]) > strlen(listenArgName)) {
          fileNameToStorePortIn = strdup(_av[i]+strlen(listenArgName)+1);
        }
        removeArgs(*_ac,_av,i,1);

        currentDevice.reset(createMpiDevice("mpi_offload"));
        currentDevice->setParam<std::string>("mpiMode", "mpi-listen");
        currentDevice->setParam<std::string>("fileNameToStorePortIn",
                                fileNameToStorePortIn?fileNameToStorePortIn:"");
        --i;
        continue;
      }

      const char *connectArgName = "--osp:mpi-connect";
      if (!strncmp(_av[i], connectArgName, strlen(connectArgName))) {
        std::string portName = _av[i+1];
        removeArgs(*_ac,_av,i,2);

        if (!currentDevice)
          currentDevice.reset(createMpiDevice("mpi_offload"));

        currentDevice->setParam<std::string>("mpiMode", "mpi-connect");
        currentDevice->setParam<std::string>("portName", portName);
        --i;
        continue;
      }
    }
  }

  // no device created on cmd line, yet, so default to ISPCDevice
  if (!deviceIsSet()) {
    ospLoadModule("ispc");
    currentDevice.reset(Device::createDevice("default"));
  }

  ospray::initFromCommandLine(_ac,&_av);

  currentDevice->commit();

  return OSP_NO_ERROR;
}
OSPRAY_CATCH_END(OSP_INVALID_OPERATION)

extern "C" void ospShutdown()
OSPRAY_CATCH_BEGIN
{
  Device::current.reset();
  LibraryRepository::cleanupInstance();
}
OSPRAY_CATCH_END()

extern "C" OSPDevice ospNewDevice(const char *deviceType)
OSPRAY_CATCH_BEGIN
{
  return (OSPDevice)Device::createDevice(deviceType);
}
OSPRAY_CATCH_END(nullptr)

// for backward compatibility, will be removed in future
extern "C" OSPDevice ospCreateDevice(const char *deviceType)
{
  return ospNewDevice(deviceType);
}

extern "C" void ospSetCurrentDevice(OSPDevice _device)
OSPRAY_CATCH_BEGIN
{
  auto *device = (Device*)_device;

  if (!device->isCommitted()) {
    throw std::runtime_error("You must commit the device before using it!");
  }

  Device::current.reset(device);
}
OSPRAY_CATCH_END()

extern "C" OSPDevice ospGetCurrentDevice()
OSPRAY_CATCH_BEGIN
{
  return (OSPDevice)Device::current.get();
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospFreeFrameBuffer(OSPFrameBuffer fb)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(fb != nullptr);
  currentDevice().release(fb);
}
OSPRAY_CATCH_END()

extern "C" OSPFrameBuffer ospNewFrameBuffer(const osp::vec2i &size,
                                            const OSPFrameBufferFormat mode,
                                            const uint32_t channels)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();

  // remove OSP_FB_VARIANCE when OSP_FB_ACCUM is not present
  uint32_t ch = channels;
  if ((channels & OSP_FB_ACCUM) == 0)
    ch &= ~OSP_FB_VARIANCE;

  return currentDevice().frameBufferCreate((const vec2i&)size, mode, ch);
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPError ospLoadModule(const char *moduleName)
OSPRAY_CATCH_BEGIN
{
  if (deviceIsSet()) {
    return (OSPError)currentDevice().loadModule(moduleName);
  } else {
    return loadLocalModule(moduleName);
  }
}
OSPRAY_CATCH_END(OSP_UNKNOWN_ERROR)

extern "C" const void *ospMapFrameBuffer(OSPFrameBuffer fb,
                                         OSPFrameBufferChannel channel)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().frameBufferMap(fb,channel);
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospUnmapFrameBuffer(const void *mapped,
                                    OSPFrameBuffer fb)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(mapped != nullptr && "invalid mapped pointer in ospUnmapFrameBuffer");
  currentDevice().frameBufferUnmap(mapped,fb);
}
OSPRAY_CATCH_END()

extern "C" OSPModel ospNewModel()
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().newModel();
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospAddGeometry(OSPModel model, OSPGeometry geometry)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(model != nullptr && "invalid model in ospAddGeometry");
  Assert(geometry != nullptr && "invalid geometry in ospAddGeometry");
  return currentDevice().addGeometry(model, geometry);
}
OSPRAY_CATCH_END()

extern "C" void ospRemoveGeometry(OSPModel model, OSPGeometry geometry)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(model != nullptr && "invalid model in ospRemoveGeometry");
  Assert(geometry != nullptr && "invalid geometry in ospRemoveGeometry");
  return currentDevice().removeGeometry(model, geometry);
}
OSPRAY_CATCH_END()

extern "C" void ospAddVolume(OSPModel model, OSPVolume volume)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(model != nullptr && "invalid model in ospAddVolume");
  Assert(volume != nullptr && "invalid volume in ospAddVolume");
  return currentDevice().addVolume(model, volume);
}
OSPRAY_CATCH_END()

extern "C" void ospRemoveVolume(OSPModel model, OSPVolume volume)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(model != nullptr && "invalid model in ospRemoveVolume");
  Assert(volume != nullptr && "invalid volume in ospRemoveVolume");
  return currentDevice().removeVolume(model, volume);
}
OSPRAY_CATCH_END()

extern "C" OSPData ospNewData(size_t nitems, OSPDataType format,
                              const void *init, const uint32_t flags)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPData data = currentDevice().newData(nitems, format, init, flags);
  return data;
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospSetData(OSPObject object, const char *bufName, OSPData data)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().setObject(object, bufName, (OSPObject)data);
}
OSPRAY_CATCH_END()

extern "C" void ospSetPixelOp(OSPFrameBuffer fb, OSPPixelOp op)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().setPixelOp(fb, op);
}
OSPRAY_CATCH_END()

extern "C" void ospSetObject(OSPObject target,
                             const char *bufName,
                             OSPObject value)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().setObject(target, bufName, value);
}
OSPRAY_CATCH_END()

extern "C" OSPPixelOp ospNewPixelOp(const char *_type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert2(_type,"invalid render type identifier in ospNewPixelOp");
  int L = strlen(_type);
  char *type = STACK_BUFFER(char, L+1);
  for (int i=0;i<=L;i++) {
    char c = _type[i];
    if (c == '-' || c == ':')
      c = '_';
    type[i] = c;
  }
  OSPPixelOp pixelOp = currentDevice().newPixelOp(type);
  return pixelOp;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPRenderer ospNewRenderer(const char *_type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();

  Assert2(_type,"invalid render type identifier in ospNewRenderer");

  std::string type(_type);
  for (size_t i = 0; i < type.size(); i++) {
    if (type[i] == '-' || type[i] == ':')
      type[i] = '_';
  }
  OSPRenderer renderer = currentDevice().newRenderer(type.c_str());
  if (renderer == nullptr) {
    postStatusMsg(1) << "#ospray: could not create renderer '" << type << "'";
  }
  return renderer;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPGeometry ospNewGeometry(const char *type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(type != nullptr && "invalid geometry type identifier in ospNewGeometry");
  OSPGeometry geometry = currentDevice().newGeometry(type);
  if (geometry == nullptr) {
    postStatusMsg(1) << "#ospray: could not create geometry '" << type << "'";
  }
  return geometry;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPMaterial ospNewMaterial(OSPRenderer renderer,
                                      const char *material_type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  auto material = currentDevice().newMaterial(renderer, material_type);
  if (material == nullptr) {
    postStatusMsg(1) << "#ospray: could not create material '"
                     << material_type << "'";
  }
  return material;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPMaterial ospNewMaterial2(const char *renderer_type,
                                       const char *material_type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  auto material = currentDevice().newMaterial(renderer_type, material_type);
  if (material == nullptr) {
    postStatusMsg(1) << "#ospray: could not create material '"
                     << material_type << "'";
  }
  return material;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPLight ospNewLight(OSPRenderer, const char *type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert2(type != nullptr, "invalid light type identifier in ospNewLight");
  OSPLight light = currentDevice().newLight(type);
  if (light == nullptr) {
    postStatusMsg(1) << "#ospray: could not create light '" << type << "'";
  }
  return light;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPLight ospNewLight2(const char */*renderer_type*/,
                                 const char *light_type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPLight light = currentDevice().newLight(light_type);
  if (light == nullptr) {
    postStatusMsg(1) << "#ospray: could not create light '"
                     << light_type << "'";
  }
  return light;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPLight ospNewLight3(const char *light_type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPLight light = currentDevice().newLight(light_type);
  if (light == nullptr) {
    postStatusMsg(1) << "#ospray: could not create light '"
                     << light_type << "'";
  }
  return light;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPCamera ospNewCamera(const char *type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(type != nullptr && "invalid camera type identifier in ospNewCamera");
  OSPCamera camera = currentDevice().newCamera(type);
  if (camera == nullptr) {
    postStatusMsg(1) << "#ospray: could not create camera '" << type << "'";
  }
  return camera;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPTexture ospNewTexture(const char *type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPTexture texture = currentDevice().newTexture(type);
  if (texture == nullptr) {
    postStatusMsg(1) << "#ospray: could not create texture '" << type << "'";
  }
  return texture;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPTexture ospNewTexture2D(const osp::vec2i &size,
                                      const OSPTextureFormat type,
                                      void *data,
                                      const uint32_t _flags)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert2(size.x > 0, "Width must be greater than 0 in ospNewTexture2D");
  Assert2(size.y > 0, "Height must be greater than 0 in ospNewTexture2D");

  auto texture = ospNewTexture("texture2d");

  if (texture == nullptr)
    return nullptr;

  auto flags = _flags; // because the input value is declared const, use a copy

  bool sharedBuffer = flags & OSP_TEXTURE_SHARED_BUFFER;

  flags &= ~OSP_TEXTURE_SHARED_BUFFER;

  const auto texelBytes  = sizeOf(type);
  const auto totalTexels = size.x * size.y;
  const auto totalBytes  = totalTexels * texelBytes;

  auto data_handle = ospNewData(totalBytes,
                                OSP_RAW,
                                data,
                                sharedBuffer ? OSP_DATA_SHARED_BUFFER : 0);

  ospCommit(data_handle);
  ospSetObject(texture, "data", data_handle);
  ospRelease(data_handle);

  ospSet1i(texture, "type", static_cast<int>(type));
  ospSet1i(texture, "flags", static_cast<int>(flags));
  ospSet2i(texture, "size", size.x, size.y);
  ospCommit(texture);

  return texture;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPVolume ospNewVolume(const char *type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(type != nullptr && "invalid volume type identifier in ospNewVolume");
  OSPVolume volume = currentDevice().newVolume(type);
  if (volume == nullptr) {
    postStatusMsg(1) << "#ospray: could not create volume '" << type << "'";
  }
  return volume;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPTransferFunction ospNewTransferFunction(const char *type)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(type != nullptr && "invalid transfer function type identifier in ospNewTransferFunction");
  auto transferFunction = currentDevice().newTransferFunction(type);
  if (transferFunction == nullptr) {
    postStatusMsg(1) << "#ospray: could not create transferFunction '" << type << "'";
  }
  return transferFunction;
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospFrameBufferClear(OSPFrameBuffer fb,
                                    const uint32_t fbChannelFlags)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().frameBufferClear(fb, fbChannelFlags);
}
OSPRAY_CATCH_END()

extern "C" float ospRenderFrame(OSPFrameBuffer fb,
                                OSPRenderer renderer,
                                const uint32_t fbChannelFlags)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().renderFrame(fb, renderer, fbChannelFlags);
}
OSPRAY_CATCH_END(inf)

extern "C" void ospCommit(OSPObject object)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(object && "invalid object handle to commit to");
  currentDevice().commit(object);
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceCommit(OSPDevice _object)
OSPRAY_CATCH_BEGIN
{
  auto *object = (Device *)_object;
  object->commit();
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSetString(OSPDevice _object,
                                   const char *id,
                                   const char *s)
OSPRAY_CATCH_BEGIN
{
  Device *object = (Device *)_object;
  object->setParam<std::string>(id, s);
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSet1b(OSPDevice _object, const char *id, int32_t x)
OSPRAY_CATCH_BEGIN
{
  Device *object = (Device *)_object;
  object->setParam(id, static_cast<bool>(x));
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSet1i(OSPDevice _object, const char *id, int32_t x)
OSPRAY_CATCH_BEGIN
{
  Device *object = (Device *)_object;
  object->setParam(id, x);
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSetVoidPtr(OSPDevice _object, const char *id, void *v)
OSPRAY_CATCH_BEGIN
{
  Device *object = (Device *)_object;
  object->setParam(id, v);
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSetStatusFunc(OSPDevice object, OSPStatusFunc callback)
OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)object;
  device->msg_fcn = callback;
}
OSPRAY_CATCH_END()

// for backward compatibility, will be removed in futur
extern "C" void ospDeviceSetErrorMsgFunc(OSPDevice dev, OSPErrorMsgFunc cb)
{
  ospDeviceSetStatusFunc(dev, cb);
}

extern "C" void ospDeviceSetErrorFunc(OSPDevice object, OSPErrorFunc callback)
OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)object;
  device->error_fcn = callback;
}
OSPRAY_CATCH_END()

extern "C" OSPError ospDeviceGetLastErrorCode(OSPDevice object)
OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)object;
  return device->lastErrorCode;
}
OSPRAY_CATCH_END(OSP_NO_ERROR)

extern "C" const char* ospDeviceGetLastErrorMsg(OSPDevice object)
OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)object;
  return device->lastErrorMsg.c_str();
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospSetProgressFunc(OSPProgressFunc callback, void* userPtr)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().progressCallback = callback;
  currentDevice().progressUserPtr = userPtr;
}
OSPRAY_CATCH_END()

extern "C" void ospSetString(OSPObject _object, const char *id, const char *s)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setString(_object, id, s);
}
OSPRAY_CATCH_END()

extern "C" void ospSet1b(OSPObject _object, const char *id, int x)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setBool(_object, id, static_cast<bool>(x));
}
OSPRAY_CATCH_END()

extern "C" void ospSetf(OSPObject _object, const char *id, float x)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setFloat(_object, id, x);
}
OSPRAY_CATCH_END()

extern "C" void ospSet1f(OSPObject _object, const char *id, float x)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setFloat(_object, id, x);
}
OSPRAY_CATCH_END()

extern "C" void ospSet1i(OSPObject _object, const char *id, int32_t x)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setInt(_object, id, x);
}
OSPRAY_CATCH_END()

extern "C" void ospSeti(OSPObject _object, const char *id, int x)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setInt(_object, id, x);
}
OSPRAY_CATCH_END()

extern "C" OSPError ospSetRegion(OSPVolume object, void *source,
                            const osp::vec3i &index, const osp::vec3i &count)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().setRegion(object, source,
                                   (const vec3i&)index, (const vec3i&)count)
    ? OSP_NO_ERROR : OSP_UNKNOWN_ERROR;
}
OSPRAY_CATCH_END(OSP_UNKNOWN_ERROR)

extern "C" void ospSetVec2f(OSPObject _object,
                            const char *id,
                            const osp::vec2f &v)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2f(_object, id, (const vec2f &)v);
}
OSPRAY_CATCH_END()

extern "C" void ospSetVec2i(OSPObject _object,
                            const char *id,
                            const osp::vec2i &v)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2i(_object, id, (const vec2i &)v);
}
OSPRAY_CATCH_END()

extern "C" void ospSetVec3f(OSPObject _object,
                            const char *id,
                            const osp::vec3f &v)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3f(_object, id, (const vec3f &)v);
}
OSPRAY_CATCH_END()

extern "C" void ospSetVec4f(OSPObject _object,
                            const char *id,
                            const osp::vec4f &v)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec4f(_object, id, (const vec4f &)v);
}
OSPRAY_CATCH_END()

extern "C" void ospSetVec3i(OSPObject _object,
                            const char *id,
                            const osp::vec3i &v)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3i(_object, id, (const vec3i &)v);
}
OSPRAY_CATCH_END()

extern "C" void ospSet2f(OSPObject _object, const char *id, float x, float y)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2f(_object, id, ospray::vec2f(x, y));
}
OSPRAY_CATCH_END()

extern "C" void ospSet2fv(OSPObject _object, const char *id, const float *xy)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2f(_object, id, vec2f(xy[0], xy[1]));
}
OSPRAY_CATCH_END()

extern "C" void ospSet2i(OSPObject _object, const char *id, int x, int y)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2i(_object, id, vec2i(x,y));
}
OSPRAY_CATCH_END()

extern "C" void ospSet2iv(OSPObject _object, const char *id, const int *xy)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2i(_object, id, vec2i(xy[0], xy[1]));
}
OSPRAY_CATCH_END()

extern "C" void ospSet3f(OSPObject _object, const char *id, float x, float y, float z)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3f(_object,id,vec3f(x,y,z));
}
OSPRAY_CATCH_END()

extern "C" void ospSet3fv(OSPObject _object, const char *id, const float *xyz)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3f(_object,id,vec3f(xyz[0],xyz[1],xyz[2]));
}
OSPRAY_CATCH_END()

extern "C" void ospSet3i(OSPObject _object, const char *id, int x, int y, int z)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3i(_object, id, vec3i(x, y, z));
}
OSPRAY_CATCH_END()

extern "C" void ospSet3iv(OSPObject _object, const char *id, const int *xyz)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3i(_object, id, vec3i(xyz[0], xyz[1], xyz[2]));
}
OSPRAY_CATCH_END()

extern "C" void ospSet4f(OSPObject _object, const char *id,
                         float x, float y, float z, float w)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec4f(_object, id, vec4f(x, y, z, w));
}
OSPRAY_CATCH_END()

extern "C" void ospSet4fv(OSPObject _object, const char *id, const float *xyzw)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec4f(_object, id,
                           vec4f(xyzw[0], xyzw[1], xyzw[2], xyzw[3]));
}
OSPRAY_CATCH_END()

extern "C" void ospSetVoidPtr(OSPObject _object, const char *id, void *v)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVoidPtr(_object, id, v);
}
OSPRAY_CATCH_END()

extern "C" void ospRemoveParam(OSPObject _object, const char *id)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().removeParam(_object, id);
}
OSPRAY_CATCH_END()

extern "C" void ospRelease(OSPObject _object)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  if (!_object) return;
  currentDevice().release(_object);
}
OSPRAY_CATCH_END()

extern "C" void ospSetMaterial(OSPGeometry geometry, OSPMaterial material)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert2(geometry,"nullptr geometry passed to ospSetMaterial");
  currentDevice().setMaterial(geometry, material);
}
OSPRAY_CATCH_END()

extern "C" OSPGeometry ospNewInstance(OSPModel modelToInstantiate,
                                      const osp::affine3f &xfm)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPGeometry geom = ospNewGeometry("instance");
  ospSet3f(geom, "xfm.l.vx", xfm.l.vx.x, xfm.l.vx.y, xfm.l.vx.z);
  ospSet3f(geom, "xfm.l.vy", xfm.l.vy.x, xfm.l.vy.y, xfm.l.vy.z);
  ospSet3f(geom, "xfm.l.vz", xfm.l.vz.x, xfm.l.vz.y, xfm.l.vz.z);
  ospSet3f(geom, "xfm.p", xfm.p.x, xfm.p.y, xfm.p.z);
  ospSetObject(geom,"model",modelToInstantiate);
  return geom;
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospPick(OSPPickResult *result,
                        OSPRenderer renderer,
                        const osp::vec2f &screenPos)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert2(renderer, "nullptr renderer passed to ospPick");
  if (!result) return;
  *result = currentDevice().pick(renderer, (const vec2f&)screenPos);
}
OSPRAY_CATCH_END()

extern "C" void ospSampleVolume(float **results,
                                OSPVolume volume,
                                const osp::vec3f &worldCoordinates,
                                const size_t count)
OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert2(volume, "nullptr volume passed to ospSampleVolume");

  if (count == 0) {
    *results = nullptr;
    return;
  }

  currentDevice().sampleVolume(results, volume,
                               (const vec3f*)&worldCoordinates, count);
}
OSPRAY_CATCH_END()
