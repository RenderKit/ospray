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

// ospcommon
#include "ospcommon/library.h"
#include "ospcommon/utility/OnScopeExit.h"
#include "ospcommon/utility/StringManip.h"
#include "ospcommon/utility/getEnvVar.h"

// ospray
#include "Device.h"
#include "common/OSPCommon.h"
#include "include/ospray/ospray.h"

#ifdef _WIN32
#include <process.h>  // for getpid
#endif

using ospray::api::currentDevice;
using ospray::api::Device;
using ospray::api::deviceIsSet;

/*! \file api.cpp implements the public ospray api functions by
  routing them to a respective \ref device */

inline std::string getPidString()
{
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
}

#define ASSERT_DEVICE()                         \
  if (!deviceIsSet())                           \
    throw std::runtime_error(                   \
        "OSPRay not yet initialized "           \
        "(most likely this means you tried to " \
        "call an ospray API function before "   \
        "first calling ospInit())" +            \
        getPidString());

#define OSPRAY_CATCH_BEGIN                 \
  try {                                    \
    auto *fcn_name_ = __PRETTY_FUNCTION__; \
    ospcommon::utility::OnScopeExit guard([&]() { postTraceMsg(fcn_name_); });

#define OSPRAY_CATCH_END(a)                                                 \
  }                                                                         \
  catch (const std::bad_alloc &)                                            \
  {                                                                         \
    handleError(OSP_OUT_OF_MEMORY, "OSPRay was unable to allocate memory"); \
    return a;                                                               \
  }                                                                         \
  catch (const std::exception &e)                                           \
  {                                                                         \
    handleError(OSP_UNKNOWN_ERROR, e.what());                               \
    return a;                                                               \
  }                                                                         \
  catch (...)                                                               \
  {                                                                         \
    handleError(OSP_UNKNOWN_ERROR, "an unrecognized exception was caught"); \
    return a;                                                               \
  }

using namespace ospray;

static inline Device *createMpiDevice(const std::string &type)
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

static inline void loadModulesFromEnvironmentVar()
{
  auto OSPRAY_LOAD_MODULES =
      utility::getEnvVar<std::string>("OSPRAY_LOAD_MODULES");

  if (OSPRAY_LOAD_MODULES) {
    auto module_names = utility::split(OSPRAY_LOAD_MODULES.value(), ',');
    for (auto &name : module_names)
      loadLocalModule(name);
  }
}

///////////////////////////////////////////////////////////////////////////////
// OSPRay Initialization //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPError ospInit(int *_ac, const char **_av) OSPRAY_CATCH_BEGIN
{
  auto &currentDevice = Device::current;

  if (currentDevice) {
    throw std::runtime_error("device already exists [ospInit() called twice?]");
  }

  loadModulesFromEnvironmentVar();

  auto OSP_MPI_LAUNCH = utility::getEnvVar<std::string>("OSPRAY_MPI_LAUNCH");

  if (OSP_MPI_LAUNCH) {
    postStatusMsg(
        "#osp: launching ospray mpi ring - "
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
        removeArgs(*_ac, _av, i, 1);
        loadLocalModule("stream");
        currentDevice.reset(Device::createDevice("stream"));

        --i;
        continue;
      }

      auto moduleSwitch = av.substr(0, 13);
      if (moduleSwitch == "--osp:module:") {
        removeArgs(*_ac, _av, i, 1);

        auto moduleName = av.substr(13);
        loadLocalModule(moduleName);

        --i;
        continue;
      }

      auto deviceSwitch = av.substr(0, 13);
      if (deviceSwitch == "--osp:device:") {
        removeArgs(*_ac, _av, i, 1);
        auto deviceName = av.substr(13);

        try {
          currentDevice.reset(Device::createDevice(deviceName.c_str()));
        } catch (const std::runtime_error &) {
          throw std::runtime_error("Failed to create device of type '" +
                                   deviceName +
                                   "'! Perhaps you spelled the "
                                   "device name wrong or didn't link the "
                                   "library which defines the device?");
        }
        --i;
        continue;
      }

      if (av == "--osp:mpi" || av == "--osp:mpi-offload") {
        removeArgs(*_ac, _av, i, 1);
        if (!currentDevice)
          currentDevice.reset(createMpiDevice("mpi_offload"));
        --i;
        continue;
      }

      if (av == "--osp:mpi-distributed") {
        removeArgs(*_ac, _av, i, 1);
        if (!currentDevice)
          currentDevice.reset(createMpiDevice("mpi_distributed"));
        --i;
        continue;
      }

      if (av == "--osp:mpi-launch") {
        if (i + 2 > *_ac)
          throw std::runtime_error("--osp:mpi-launch expects an argument");
        currentDevice.reset(createMpiDevice("mpi_offload"));
        currentDevice->setParam<std::string>("mpiMode", "mpi-launch");
        currentDevice->setParam<std::string>("launchCommand", _av[i + 1]);
        removeArgs(*_ac, _av, i, 2);
        --i;
        continue;
      }

      const char *listenArgName = "--osp:mpi-listen";
      if (!strncmp(_av[i], listenArgName, strlen(listenArgName))) {
        const char *fileNameToStorePortIn = nullptr;
        if (strlen(_av[i]) > strlen(listenArgName)) {
          fileNameToStorePortIn = strdup(_av[i] + strlen(listenArgName) + 1);
        }
        removeArgs(*_ac, _av, i, 1);

        currentDevice.reset(createMpiDevice("mpi_offload"));
        currentDevice->setParam<std::string>("mpiMode", "mpi-listen");
        currentDevice->setParam<std::string>(
            "fileNameToStorePortIn",
            fileNameToStorePortIn ? fileNameToStorePortIn : "");
        --i;
        continue;
      }

      const char *connectArgName = "--osp:mpi-connect";
      if (!strncmp(_av[i], connectArgName, strlen(connectArgName))) {
        std::string portName = _av[i + 1];
        removeArgs(*_ac, _av, i, 2);

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
    auto OSPRAY_DEFAULT_DEVICE =
        utility::getEnvVar<std::string>("OSPRAY_DEFAULT_DEVICE");

    if (OSPRAY_DEFAULT_DEVICE) {
      auto device_name = OSPRAY_DEFAULT_DEVICE.value();
      currentDevice.reset(Device::createDevice(device_name.c_str()));
    } else {
      ospLoadModule("ispc");
      currentDevice.reset(Device::createDevice("default"));
    }
  }

  ospray::initFromCommandLine(_ac, &_av);

  currentDevice->commit();

  return OSP_NO_ERROR;
}
OSPRAY_CATCH_END(OSP_INVALID_OPERATION)

extern "C" void ospShutdown() OSPRAY_CATCH_BEGIN
{
  Device::current.reset();
  LibraryRepository::cleanupInstance();
}
OSPRAY_CATCH_END()

extern "C" OSPDevice ospNewDevice(const char *deviceType) OSPRAY_CATCH_BEGIN
{
  return (OSPDevice)Device::createDevice(deviceType);
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospSetCurrentDevice(OSPDevice _device) OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)_device;

  if (!device->isCommitted()) {
    throw std::runtime_error("You must commit the device before using it!");
  }

  Device::current.reset(device);
}
OSPRAY_CATCH_END()

extern "C" OSPDevice ospGetCurrentDevice() OSPRAY_CATCH_BEGIN
{
  return (OSPDevice)Device::current.get();
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospDeviceSetString(OSPDevice _object,
                                   const char *id,
                                   const char *s) OSPRAY_CATCH_BEGIN
{
  Device *object = (Device *)_object;
  object->setParam<std::string>(id, s);
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSet1b(OSPDevice _object,
                               const char *id,
                               int x) OSPRAY_CATCH_BEGIN
{
  Device *object = (Device *)_object;
  object->setParam(id, static_cast<bool>(x));
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSet1i(OSPDevice _object,
                               const char *id,
                               int x) OSPRAY_CATCH_BEGIN
{
  Device *object = (Device *)_object;
  object->setParam(id, x);
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSetVoidPtr(OSPDevice _object,
                                    const char *id,
                                    void *v) OSPRAY_CATCH_BEGIN
{
  Device *object = (Device *)_object;
  object->setParam(id, v);
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSetStatusFunc(OSPDevice object, OSPStatusFunc callback)
    OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)object;
  if (callback == nullptr)
    device->msg_fcn = [](const char *) {};
  else
    device->msg_fcn = callback;
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSetErrorFunc(OSPDevice object,
                                      OSPErrorFunc callback) OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)object;
  if (callback == nullptr)
    device->error_fcn = [](OSPError, const char *) {};
  else
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

extern "C" const char *ospDeviceGetLastErrorMsg(OSPDevice object)
    OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)object;
  return device->lastErrorMsg.c_str();
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospDeviceCommit(OSPDevice _object) OSPRAY_CATCH_BEGIN
{
  auto *object = (Device *)_object;
  object->commit();
}
OSPRAY_CATCH_END()

extern "C" OSPError ospLoadModule(const char *moduleName) OSPRAY_CATCH_BEGIN
{
  if (deviceIsSet()) {
    return (OSPError)currentDevice().loadModule(moduleName);
  } else {
    return loadLocalModule(moduleName);
  }
}
OSPRAY_CATCH_END(OSP_UNKNOWN_ERROR)

///////////////////////////////////////////////////////////////////////////////
// OSPRay Data Arrays /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPData ospNewData(size_t nitems,
                              OSPDataType format,
                              const void *init,
                              uint32_t flags) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPData data = currentDevice().newData(nitems, format, init, flags);
  return data;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPError ospSetRegion(OSPVolume object,
                                 void *source,
                                 osp_vec3i index,
                                 osp_vec3i count) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().setRegion(
             object, source, (const vec3i &)index, (const vec3i &)count)
             ? OSP_NO_ERROR
             : OSP_UNKNOWN_ERROR;
}
OSPRAY_CATCH_END(OSP_UNKNOWN_ERROR)

///////////////////////////////////////////////////////////////////////////////
// Renderable Objects /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPLight ospNewLight(const char *light_type) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPLight light = currentDevice().newLight(light_type);
  if (light == nullptr) {
    postStatusMsg(1) << "#ospray: could not create light '" << light_type
                     << "'";
  }
  return light;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPCamera ospNewCamera(const char *type) OSPRAY_CATCH_BEGIN
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

extern "C" OSPGeometry ospNewGeometry(const char *type) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(type != nullptr &&
         "invalid geometry type identifier in ospNewGeometry");
  OSPGeometry geometry = currentDevice().newGeometry(type);
  if (geometry == nullptr) {
    postStatusMsg(1) << "#ospray: could not create geometry '" << type << "'";
  }
  return geometry;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPVolume ospNewVolume(const char *type) OSPRAY_CATCH_BEGIN
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

///////////////////////////////////////////////////////////////////////////////
// Instancing /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPGeometryInstance ospNewGeometryInstance(OSPGeometry geom)
    OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPGeometryInstance instance = currentDevice().newGeometryInstance(geom);
  return instance;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPVolumeInstance ospNewVolumeInstance(OSPVolume volume)
    OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPVolumeInstance instance = currentDevice().newVolumeInstance(volume);
  return instance;
}
OSPRAY_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// Instance Meta-Data /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPMaterial ospNewMaterial(
    const char *renderer_type, const char *material_type) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  auto material = currentDevice().newMaterial(renderer_type, material_type);
  if (material == nullptr) {
    postStatusMsg(1) << "#ospray: could not create material '" << material_type
                     << "'";
  }
  return material;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPTransferFunction ospNewTransferFunction(const char *type)
    OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(type != nullptr &&
         "invalid transfer function type identifier in ospNewTransferFunction");
  auto transferFunction = currentDevice().newTransferFunction(type);
  if (transferFunction == nullptr) {
    postStatusMsg(1) << "#ospray: could not create transferFunction '" << type
                     << "'";
  }
  return transferFunction;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPTexture ospNewTexture(const char *type) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPTexture texture = currentDevice().newTexture(type);
  if (texture == nullptr) {
    postStatusMsg(1) << "#ospray: could not create texture '" << type << "'";
  }
  return texture;
}
OSPRAY_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// World Manipulation /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPWorld ospNewWorld() OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().newWorld();
}
OSPRAY_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// Object Parameters //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void ospSetString(OSPObject _object,
                             const char *id,
                             const char *s) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setString(_object, id, s);
}
OSPRAY_CATCH_END()

extern "C" void ospSetObject(OSPObject target,
                             const char *bufName,
                             OSPObject value) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().setObject(target, bufName, value);
}
OSPRAY_CATCH_END()

extern "C" void ospSetData(OSPObject object,
                           const char *bufName,
                           OSPData data) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().setObject(object, bufName, (OSPObject)data);
}
OSPRAY_CATCH_END()

extern "C" void ospSet1b(OSPObject _object,
                         const char *id,
                         int x) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setBool(_object, id, static_cast<bool>(x));
}
OSPRAY_CATCH_END()

extern "C" void ospSet1f(OSPObject _object,
                         const char *id,
                         float x) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setFloat(_object, id, x);
}
OSPRAY_CATCH_END()

extern "C" void ospSet1i(OSPObject _object,
                         const char *id,
                         int x) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setInt(_object, id, x);
}
OSPRAY_CATCH_END()

extern "C" void ospSeti(OSPObject _object,
                        const char *id,
                        int x) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setInt(_object, id, x);
}
OSPRAY_CATCH_END()

extern "C" void ospSet2f(OSPObject _object, const char *id, float x, float y)
    OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2f(_object, id, ospray::vec2f(x, y));
}
OSPRAY_CATCH_END()

extern "C" void ospSet2fv(OSPObject _object,
                          const char *id,
                          const float *xy) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2f(_object, id, vec2f(xy[0], xy[1]));
}
OSPRAY_CATCH_END()

extern "C" void ospSet2i(OSPObject _object, const char *id, int x, int y)
    OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2i(_object, id, vec2i(x, y));
}
OSPRAY_CATCH_END()

extern "C" void ospSet2iv(OSPObject _object,
                          const char *id,
                          const int *xy) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec2i(_object, id, vec2i(xy[0], xy[1]));
}
OSPRAY_CATCH_END()

extern "C" void ospSet3f(OSPObject _object,
                         const char *id,
                         float x,
                         float y,
                         float z) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3f(_object, id, vec3f(x, y, z));
}
OSPRAY_CATCH_END()

extern "C" void ospSet3fv(OSPObject _object,
                          const char *id,
                          const float *xyz) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3f(_object, id, vec3f(xyz[0], xyz[1], xyz[2]));
}
OSPRAY_CATCH_END()

extern "C" void ospSet3i(OSPObject _object, const char *id, int x, int y, int z)
    OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3i(_object, id, vec3i(x, y, z));
}
OSPRAY_CATCH_END()

extern "C" void ospSet3iv(OSPObject _object,
                          const char *id,
                          const int *xyz) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec3i(_object, id, vec3i(xyz[0], xyz[1], xyz[2]));
}
OSPRAY_CATCH_END()

extern "C" void ospSet4f(OSPObject _object,
                         const char *id,
                         float x,
                         float y,
                         float z,
                         float w) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec4f(_object, id, vec4f(x, y, z, w));
}
OSPRAY_CATCH_END()

extern "C" void ospSet4fv(OSPObject _object,
                          const char *id,
                          const float *xyzw) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVec4f(
      _object, id, vec4f(xyzw[0], xyzw[1], xyzw[2], xyzw[3]));
}
OSPRAY_CATCH_END()

extern "C" void ospSetVoidPtr(OSPObject _object,
                              const char *id,
                              void *v) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setVoidPtr(_object, id, v);
}
OSPRAY_CATCH_END()

extern "C" void ospRemoveParam(OSPObject _object,
                               const char *id) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().removeParam(_object, id);
}
OSPRAY_CATCH_END()

extern "C" void ospSetMaterial(OSPGeometryInstance instance,
                               OSPMaterial material) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().setMaterial(instance, material);
}
OSPRAY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Object + Parameter Lifetime Management /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void ospCommit(OSPObject object) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(object && "invalid object handle to commit to");
  currentDevice().commit(object);
}
OSPRAY_CATCH_END()

extern "C" void ospRelease(OSPObject _object) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  if (!_object)
    return;
  currentDevice().release(_object);
}
OSPRAY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// FrameBuffer Manipulation ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPFrameBuffer ospNewFrameBuffer(osp_vec2i size,
                                            OSPFrameBufferFormat mode,
                                            uint32_t channels)
    OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();

  // remove OSP_FB_VARIANCE when OSP_FB_ACCUM is not present
  uint32_t ch = channels;
  if ((channels & OSP_FB_ACCUM) == 0)
    ch &= ~OSP_FB_VARIANCE;

  return currentDevice().frameBufferCreate((const vec2i &)size, mode, ch);
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPPixelOp ospNewPixelOp(const char *_type) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert2(_type, "invalid render type identifier in ospNewPixelOp");
  int L      = strlen(_type);
  char *type = STACK_BUFFER(char, L + 1);
  for (int i = 0; i <= L; i++) {
    char c = _type[i];
    if (c == '-' || c == ':')
      c = '_';
    type[i] = c;
  }
  OSPPixelOp pixelOp = currentDevice().newPixelOp(type);
  return pixelOp;
}
OSPRAY_CATCH_END(nullptr)

extern "C" const void *ospMapFrameBuffer(
    OSPFrameBuffer fb, OSPFrameBufferChannel channel) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().frameBufferMap(fb, channel);
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospUnmapFrameBuffer(const void *mapped,
                                    OSPFrameBuffer fb) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert(mapped != nullptr && "invalid mapped pointer in ospUnmapFrameBuffer");
  currentDevice().frameBufferUnmap(mapped, fb);
}
OSPRAY_CATCH_END()

extern "C" float ospGetVariance(OSPFrameBuffer f) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().getVariance(f);
}
OSPRAY_CATCH_END(inf)

extern "C" void ospResetAccumulation(OSPFrameBuffer fb) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().resetAccumulation(fb);
}
OSPRAY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Frame Rendering ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPRenderer ospNewRenderer(const char *_type) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();

  Assert2(_type, "invalid render type identifier in ospNewRenderer");

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

extern "C" float ospRenderFrame(OSPFrameBuffer fb,
                                OSPRenderer renderer,
                                OSPCamera camera,
                                OSPWorld world) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().renderFrame(fb, renderer, camera, world);
}
OSPRAY_CATCH_END(inf)

extern "C" OSPFuture ospRenderFrameAsync(OSPFrameBuffer fb,
                                         OSPRenderer renderer,
                                         OSPCamera camera,
                                         OSPWorld world) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().renderFrameAsync(fb, renderer, camera, world);
}
OSPRAY_CATCH_END(nullptr)

extern "C" int ospIsReady(OSPFuture f, OSPSyncEvent event) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().isReady(f, event);
}
OSPRAY_CATCH_END(1)

extern "C" void ospWait(OSPFuture f, OSPSyncEvent et) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  if (et == OSP_NONE_FINISHED)
    return;
  currentDevice().wait(f, et);
}
OSPRAY_CATCH_END()

extern "C" void ospCancel(OSPFuture f) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().cancel(f);
}
OSPRAY_CATCH_END()

extern "C" float ospGetProgress(OSPFuture f) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().getProgress(f);
}
OSPRAY_CATCH_END(1.f)

extern "C" void ospPick(OSPPickResult *result,
                        OSPFrameBuffer fb,
                        OSPRenderer renderer,
                        OSPCamera camera,
                        OSPWorld world,
                        osp_vec2f screenPos) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  Assert2(renderer, "nullptr renderer passed to ospPick");
  if (!result)
    return;
  *result = currentDevice().pick(
      fb, renderer, camera, world, (const vec2f &)screenPos);
}
OSPRAY_CATCH_END()
