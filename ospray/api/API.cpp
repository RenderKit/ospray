// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospcommon
#include "ospcommon/os/library.h"
#include "ospcommon/utility/OnScopeExit.h"
#include "ospcommon/utility/StringManip.h"
#include "ospcommon/utility/getEnvVar.h"

// ospray
#include "Device.h"
#include "common/OSPCommon.h"
#include "include/ospray/ospray.h"

#ifdef _WIN32
#include <process.h> // for getpid
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

#define THROW_IF_NULL(obj, name)                                               \
  if (obj == nullptr)                                                          \
  throw std::runtime_error(std::string("null ") + name                         \
      + std::string(" provided to ") + __FUNCTION__)

// convenience macros for repeated use of the above
#define THROW_IF_NULL_OBJECT(obj) THROW_IF_NULL(obj, "handle")
#define THROW_IF_NULL_STRING(str) THROW_IF_NULL(str, "string")

#define ASSERT_DEVICE()                                                        \
  if (!deviceIsSet())                                                          \
  throw std::runtime_error(                                                    \
      "OSPRay not yet initialized "                                            \
      "(most likely this means you tried to "                                  \
      "call an ospray API function before "                                    \
      "first calling ospInit())"                                               \
      + getPidString())

#define OSPRAY_CATCH_BEGIN                                                     \
  try {                                                                        \
    auto *fcn_name_ = __PRETTY_FUNCTION__;                                     \
    ospcommon::utility::OnScopeExit guard([&]() { postTraceMsg(fcn_name_); });

#define OSPRAY_CATCH_END(a)                                                    \
  }                                                                            \
  catch (const std::bad_alloc &)                                               \
  {                                                                            \
    handleError(OSP_OUT_OF_MEMORY, "OSPRay was unable to allocate memory");    \
    return a;                                                                  \
  }                                                                            \
  catch (const std::exception &e)                                              \
  {                                                                            \
    handleError(OSP_UNKNOWN_ERROR, e.what());                                  \
    return a;                                                                  \
  }                                                                            \
  catch (...)                                                                  \
  {                                                                            \
    handleError(OSP_UNKNOWN_ERROR, "an unrecognized exception was caught");    \
    return a;                                                                  \
  }

using namespace ospray;

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

  if (_ac && _av) {
    for (int i = 1; i < *_ac;) {
      std::string av(_av[i]);

      if (ospcommon::utility::beginsWith(av, "--osp:load-modules")) {
        std::string modules = getArgString(av);
        if (modules == "") {
          throw std::runtime_error(
              "Invalid module name(s) provided to --osp:load-modules. Must be "
              "formatted as <module1>[,<module2>,...]");
        }
        std::vector<std::string> moduleList =
            ospcommon::utility::split(modules, ',');
        for (std::string &moduleName : moduleList) {
          loadLocalModule(moduleName);
        }
        removeArgs(*_ac, _av, i, 1);
      } else if (ospcommon::utility::beginsWith(av, "--osp:device=")) {
        // ALOK: explicitly checking with the equals sign so this does not get
        // clobbered by --osp:device-param
        std::string deviceName = getArgString(av);
        try {
          currentDevice = Device::createDevice(deviceName.c_str());
        } catch (const std::runtime_error &) {
          throw std::runtime_error("Failed to create device of type '" +
                                   deviceName +
                                   "'! Perhaps you spelled the "
                                   "device name wrong or didn't link the "
                                   "library which defines the device?");
        }
        removeArgs(*_ac, _av, i, 1);
      } else {
        // Ignore other arguments
        ++i;
      }
    }
  }

  // no device created on cmd line, yet, so default to ISPCDevice
  if (!deviceIsSet()) {
    auto OSPRAY_DEVICE = utility::getEnvVar<std::string>("OSPRAY_DEVICE");

    if (OSPRAY_DEVICE) {
      auto device_name = OSPRAY_DEVICE.value();
      currentDevice = Device::createDevice(device_name.c_str());
    } else {
      ospLoadModule("ispc");
      currentDevice = Device::createDevice("cpu");
    }
  }

  ospray::initFromCommandLine(_ac, &_av);

  currentDevice->commit();
  currentDevice->refDec();

  return OSP_NO_ERROR;
}
OSPRAY_CATCH_END(OSP_INVALID_OPERATION)

extern "C" int64_t ospDeviceGetProperty(
    OSPDevice _device, OSPDeviceProperty _deviceProperty) OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)_device;
  return device->getProperty(_deviceProperty);
}
OSPRAY_CATCH_END(0)

extern "C" void ospShutdown() OSPRAY_CATCH_BEGIN
{
  Device::current = nullptr;
  LibraryRepository::cleanupInstance();
}
OSPRAY_CATCH_END()

extern "C" OSPDevice ospNewDevice(const char *deviceType) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(deviceType);
  return (OSPDevice)Device::createDevice(deviceType);
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospSetCurrentDevice(OSPDevice _device) OSPRAY_CATCH_BEGIN
{
  auto *device = (Device *)_device;

  if (!device->isCommitted()) {
    throw std::runtime_error("You must commit the device before using it!");
  }

  Device::current = device;
}
OSPRAY_CATCH_END()

extern "C" OSPDevice ospGetCurrentDevice() OSPRAY_CATCH_BEGIN
{
  auto *ptr = Device::current.ptr;
  ptr->refInc();
  return (OSPDevice)ptr;
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospDeviceSetParam(OSPDevice _object,
    const char *id,
    OSPDataType type,
    const void *mem) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);
  THROW_IF_NULL_STRING(id);
  Device *object = (Device *)_object;

  switch (type) {
  case OSP_STRING:
    object->setParam<std::string>(id, std::string((const char *)mem));
    break;
  case OSP_INT:
    object->setParam<int>(id, *(int *)mem);
    break;
  case OSP_BOOL:
    object->setParam<bool>(id, *(int *)mem);
    break;
  case OSP_VOID_PTR:
    object->setParam<void *>(id, *(void **)&mem);
    break;
  default:
    throw std::runtime_error("parameter type not handled for OSPDevice!");
  };
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceRemoveParam(
    OSPDevice _object, const char *id) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);
  THROW_IF_NULL_STRING(id);
  Device *object = (Device *)_object;
  object->removeParam(id);
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSetStatusFunc(
    OSPDevice _object, OSPStatusFunc callback) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);

  auto *device = (Device *)_object;
  if (callback == nullptr)
    device->msg_fcn = [](const char *) {};
  else
    device->msg_fcn = callback;
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceSetErrorFunc(
    OSPDevice _object, OSPErrorFunc callback) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);

  auto *device = (Device *)_object;
  if (callback == nullptr)
    device->error_fcn = [](OSPError, const char *) {};
  else
    device->error_fcn = callback;
}
OSPRAY_CATCH_END()

extern "C" OSPError ospDeviceGetLastErrorCode(
    OSPDevice _object) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);

  auto *device = (Device *)_object;
  return device->lastErrorCode;
}
OSPRAY_CATCH_END(OSP_NO_ERROR)

extern "C" const char *ospDeviceGetLastErrorMsg(
    OSPDevice _object) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);

  auto *device = (Device *)_object;
  return device->lastErrorMsg.c_str();
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospDeviceCommit(OSPDevice _object) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);

  auto *object = (Device *)_object;
  object->commit();
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceRelease(OSPDevice _object) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);

  auto *object = (Device *)_object;
  object->refDec();
}
OSPRAY_CATCH_END()

extern "C" void ospDeviceRetain(OSPDevice _object) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);

  auto *object = (Device *)_object;
  object->refInc();
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

extern "C" OSPData ospNewSharedData(const void *sharedData,
    OSPDataType type,
    uint64_t numItems1,
    int64_t byteStride1,
    uint64_t numItems2,
    int64_t byteStride2,
    uint64_t numItems3,
    int64_t byteStride3) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPData data = currentDevice().newSharedData(sharedData,
      type,
      ospray::vec3ul(numItems1, numItems2, numItems3),
      ospray::vec3l(byteStride1, byteStride2, byteStride3));
  return data;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPData ospNewData(OSPDataType type,
    uint64_t numItems1,
    uint64_t numItems2,
    uint64_t numItems3) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  OSPData data = currentDevice().newData(
      type, ospray::vec3ul(numItems1, numItems2, numItems3));
  return data;
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospCopyData(const OSPData source,
    OSPData destination,
    uint64_t dstIdx1,
    uint64_t dstIdx2,
    uint64_t dstIdx3) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  currentDevice().copyData(
      source, destination, ospray::vec3ul(dstIdx1, dstIdx2, dstIdx3));
}
OSPRAY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Renderable Objects /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPLight ospNewLight(const char *_type) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(_type);
  ASSERT_DEVICE();
  OSPLight light = currentDevice().newLight(_type);
  return light;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPCamera ospNewCamera(const char *_type) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(_type);
  ASSERT_DEVICE();
  OSPCamera camera = currentDevice().newCamera(_type);
  return camera;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPGeometry ospNewGeometry(const char *_type) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(_type);
  ASSERT_DEVICE();
  OSPGeometry geometry = currentDevice().newGeometry(_type);
  return geometry;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPVolume ospNewVolume(const char *_type) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(_type);
  ASSERT_DEVICE();
  OSPVolume volume = currentDevice().newVolume(_type);
  return volume;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPGeometricModel ospNewGeometricModel(
    OSPGeometry geom) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(geom);
  ASSERT_DEVICE();
  OSPGeometricModel instance = currentDevice().newGeometricModel(geom);
  return instance;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPVolumetricModel ospNewVolumetricModel(
    OSPVolume volume) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(volume);
  ASSERT_DEVICE();
  OSPVolumetricModel instance = currentDevice().newVolumetricModel(volume);
  return instance;
}
OSPRAY_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// Model Meta-Data ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPMaterial ospNewMaterial(
    const char *renderer_type, const char *material_type) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(renderer_type);
  THROW_IF_NULL_STRING(material_type);

  ASSERT_DEVICE();
  auto material = currentDevice().newMaterial(renderer_type, material_type);
  return material;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPTransferFunction ospNewTransferFunction(
    const char *_type) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(_type);

  ASSERT_DEVICE();
  auto transferFunction = currentDevice().newTransferFunction(_type);
  return transferFunction;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPTexture ospNewTexture(const char *_type) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(_type);

  ASSERT_DEVICE();
  OSPTexture texture = currentDevice().newTexture(_type);
  return texture;
}
OSPRAY_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// Instancing /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPGroup ospNewGroup() OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().newGroup();
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPInstance ospNewInstance(OSPGroup group) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(group);
  ASSERT_DEVICE();
  return currentDevice().newInstance(group);
}
OSPRAY_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// Top-level Worlds ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPWorld ospNewWorld() OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return currentDevice().newWorld();
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPBounds ospGetBounds(OSPObject obj) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(obj);
  ASSERT_DEVICE();
  box3f bounds = currentDevice().getBounds(obj);
  return (OSPBounds &)bounds;
}
OSPRAY_CATCH_END({})

///////////////////////////////////////////////////////////////////////////////
// Object + Parameter Lifetime Management /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void ospSetParam(OSPObject _object,
    const char *id,
    OSPDataType type,
    const void *mem) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);
  THROW_IF_NULL_STRING(id);
  ASSERT_DEVICE();
  currentDevice().setObjectParam(_object, id, type, mem);
}
OSPRAY_CATCH_END()

extern "C" void ospRemoveParam(
    OSPObject _object, const char *id) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);
  THROW_IF_NULL_STRING(id);
  ASSERT_DEVICE();
  currentDevice().removeObjectParam(_object, id);
}
OSPRAY_CATCH_END()

extern "C" void ospCommit(OSPObject _object) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);
  ASSERT_DEVICE();
  currentDevice().commit(_object);
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

extern "C" void ospRetain(OSPObject _object) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(_object);
  ASSERT_DEVICE();
  if (!_object)
    return;
  currentDevice().retain(_object);
}
OSPRAY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// FrameBuffer Manipulation ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPFrameBuffer ospNewFrameBuffer(int size_x,
    int size_y,
    OSPFrameBufferFormat mode,
    uint32_t channels) OSPRAY_CATCH_BEGIN
{
  ASSERT_DEVICE();

  // remove OSP_FB_VARIANCE when OSP_FB_ACCUM is not present
  uint32_t ch = channels;
  if ((channels & OSP_FB_ACCUM) == 0)
    ch &= ~OSP_FB_VARIANCE;

  return currentDevice().frameBufferCreate(vec2i(size_x, size_y), mode, ch);
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPImageOperation ospNewImageOperation(
    const char *_type) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(_type);

  ASSERT_DEVICE();
  int L = strlen(_type);
  char *type = STACK_BUFFER(char, L + 1);
  for (int i = 0; i <= L; i++) {
    char c = _type[i];
    if (c == '-' || c == ':')
      c = '_';
    type[i] = c;
  }
  OSPImageOperation op = currentDevice().newImageOp(type);
  return op;
}
OSPRAY_CATCH_END(nullptr)

extern "C" const void *ospMapFrameBuffer(
    OSPFrameBuffer fb, OSPFrameBufferChannel channel) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(fb);
  ASSERT_DEVICE();
  return currentDevice().frameBufferMap(fb, channel);
}
OSPRAY_CATCH_END(nullptr)

extern "C" void ospUnmapFrameBuffer(
    const void *mapped, OSPFrameBuffer fb) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(fb);
  THROW_IF_NULL(mapped, "pointer");
  ASSERT_DEVICE();
  currentDevice().frameBufferUnmap(mapped, fb);
}
OSPRAY_CATCH_END()

extern "C" float ospGetVariance(OSPFrameBuffer fb) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(fb);
  ASSERT_DEVICE();
  return currentDevice().getVariance(fb);
}
OSPRAY_CATCH_END(inf)

extern "C" void ospResetAccumulation(OSPFrameBuffer fb) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(fb);
  ASSERT_DEVICE();
  currentDevice().resetAccumulation(fb);
}
OSPRAY_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Frame Rendering ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OSPRenderer ospNewRenderer(const char *_type) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(_type);
  ASSERT_DEVICE();

  std::string type(_type);
  for (size_t i = 0; i < type.size(); i++) {
    if (type[i] == '-' || type[i] == ':')
      type[i] = '_';
  }
  OSPRenderer renderer = currentDevice().newRenderer(type.c_str());
  return renderer;
}
OSPRAY_CATCH_END(nullptr)

extern "C" OSPFuture ospRenderFrame(OSPFrameBuffer fb,
    OSPRenderer renderer,
    OSPCamera camera,
    OSPWorld world) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL(fb, "framebuffer");
  THROW_IF_NULL(renderer, "renderer");
  THROW_IF_NULL(camera, "camera");
  THROW_IF_NULL(world, "world");

  ASSERT_DEVICE();
  return currentDevice().renderFrame(fb, renderer, camera, world);
}
OSPRAY_CATCH_END(nullptr)

extern "C" int ospIsReady(OSPFuture f, OSPSyncEvent event) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(f);
  ASSERT_DEVICE();
  return currentDevice().isReady(f, event);
}
OSPRAY_CATCH_END(1)

extern "C" void ospWait(OSPFuture f, OSPSyncEvent event) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(f);
  ASSERT_DEVICE();
  if (event == OSP_NONE_FINISHED)
    return;
  currentDevice().wait(f, event);
}
OSPRAY_CATCH_END()

extern "C" void ospCancel(OSPFuture f) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(f);
  ASSERT_DEVICE();
  return currentDevice().cancel(f);
}
OSPRAY_CATCH_END()

extern "C" float ospGetProgress(OSPFuture f) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(f);
  ASSERT_DEVICE();
  return currentDevice().getProgress(f);
}
OSPRAY_CATCH_END(1.f)

extern "C" float ospGetTaskDuration(OSPFuture f) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(f);
  ASSERT_DEVICE();
  return currentDevice().getTaskDuration(f);
}
OSPRAY_CATCH_END(0.f)

extern "C" void ospPick(OSPPickResult *result,
    OSPFrameBuffer fb,
    OSPRenderer renderer,
    OSPCamera camera,
    OSPWorld world,
    float screenPos_x,
    float screenPos_y) OSPRAY_CATCH_BEGIN
{
  THROW_IF_NULL(fb, "framebuffer");
  THROW_IF_NULL(renderer, "renderer");
  THROW_IF_NULL(camera, "camera");
  THROW_IF_NULL(world, "world");

  ASSERT_DEVICE();
  if (!result)
    return;
  *result = currentDevice().pick(
      fb, renderer, camera, world, vec2f(screenPos_x, screenPos_y));
}
OSPRAY_CATCH_END()
