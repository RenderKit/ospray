// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "ISPCDevice.h"
#include "camera/Camera.h"
#include "common/Data.h"
#include "common/Group.h"
#include "common/Instance.h"
#include "common/Library.h"
#include "common/Util.h"
#include "common/World.h"
#include "fb/ImageOp.h"
#include "fb/LocalFB.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "render/LoadBalancer.h"
#include "render/Material.h"
#include "render/RenderTask.h"
#include "render/Renderer.h"
#include "texture/Texture.h"
#include "texture/Texture2D.h"
#include "volume/VolumetricModel.h"
#include "volume/transferFunction/TransferFunction.h"

#include "camera/registration.h"
#include "fb/registration.h"
#include "geometry/registration.h"
#include "lights/registration.h"
#include "render/registration.h"
#include "texture/registration.h"
#include "volume/transferFunction/registration.h"

// stl
#include <algorithm>
#include <functional>
#include <map>
// ospcommon
#include "ospcommon/tasking/tasking_system_init.h"
#include "ospcommon/utility/CodeTimer.h"

#include "ISPCDevice_ispc.h"

extern "C" RTCDevice ispc_embreeDevice()
{
  return ospray::api::ISPCDevice::embreeDevice;
}

namespace ospray {
namespace api {

using SetParamFcn = void(OSPObject, const char *, const void *);

template <typename T>
static void setParamOnObject(OSPObject _obj, const char *p, const T &v)
{
  auto *obj = (ManagedObject *)_obj;
  obj->setParam(p, v);
}

#define declare_param_setter(TYPE)                                             \
  {                                                                            \
    OSPTypeFor<TYPE>::value, [](OSPObject o, const char *p, const void *v) {   \
      setParamOnObject(o, p, *(TYPE *)v);                                      \
    }                                                                          \
  }

#define declare_param_setter_object(TYPE)                                      \
  {                                                                            \
    OSPTypeFor<TYPE>::value, [](OSPObject o, const char *p, const void *v) {   \
      ManagedObject *obj = *(TYPE *)v;                                         \
      setParamOnObject(o, p, obj);                                             \
    }                                                                          \
  }

#define declare_param_setter_string(TYPE)                                      \
  {                                                                            \
    OSPTypeFor<TYPE>::value, [](OSPObject o, const char *p, const void *v) {   \
      const char *str = (const char *)v;                                       \
      setParamOnObject(o, p, std::string(str));                                \
    }                                                                          \
  }

static std::map<OSPDataType, std::function<SetParamFcn>> setParamFcns = {
    declare_param_setter(Device *),
    declare_param_setter(void *),
    declare_param_setter(bool),
    declare_param_setter_object(ManagedObject *),
    declare_param_setter_object(Camera *),
    declare_param_setter_object(Data *),
    declare_param_setter_object(FrameBuffer *),
    declare_param_setter_object(Future *),
    declare_param_setter_object(GeometricModel *),
    declare_param_setter_object(Group *),
    declare_param_setter_object(ImageOp *),
    declare_param_setter_object(Instance *),
    declare_param_setter_object(Light *),
    declare_param_setter_object(Material *),
    declare_param_setter_object(Renderer *),
    declare_param_setter_object(Texture *),
    declare_param_setter_object(TransferFunction *),
    declare_param_setter_object(Volume *),
    declare_param_setter_object(VolumetricModel *),
    declare_param_setter_object(World *),
    declare_param_setter_string(const char *),
    declare_param_setter(char *),
    declare_param_setter(char),
    declare_param_setter(unsigned char),
    declare_param_setter(vec2uc),
    declare_param_setter(vec3uc),
    declare_param_setter(vec4uc),
    declare_param_setter(short),
    declare_param_setter(int),
    declare_param_setter(vec2i),
    declare_param_setter(vec3i),
    declare_param_setter(vec4i),
    declare_param_setter(unsigned int),
    declare_param_setter(vec2ui),
    declare_param_setter(vec3ui),
    declare_param_setter(vec4ui),
    declare_param_setter(int64_t),
    declare_param_setter(vec2l),
    declare_param_setter(vec3l),
    declare_param_setter(vec4l),
    declare_param_setter(uint64_t),
    declare_param_setter(vec2ul),
    declare_param_setter(vec3ul),
    declare_param_setter(vec4ul),
    declare_param_setter(float),
    declare_param_setter(vec2f),
    declare_param_setter(vec3f),
    declare_param_setter(vec4f),
    declare_param_setter(double),
    declare_param_setter(box1i),
    declare_param_setter(box2i),
    declare_param_setter(box3i),
    declare_param_setter(box4i),
    declare_param_setter(box1f),
    declare_param_setter(box2f),
    declare_param_setter(box3f),
    declare_param_setter(box4f),
    declare_param_setter(linear2f),
    declare_param_setter(linear3f),
    declare_param_setter(affine2f),
    declare_param_setter(affine3f)};

#undef declare_param_setter

RTCDevice ISPCDevice::embreeDevice = nullptr;
VKLDriver ISPCDevice::vklDriver = nullptr;

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

  if (vklDriver) {
    vklShutdown();
    vklDriver = nullptr;
  }
}

static void embreeErrorFunc(void *, const RTCError code, const char *str)
{
  postStatusMsg() << "#osp: embree internal error " << code << " : " << str;
  throw std::runtime_error("embree internal error '" + std::string(str) + "'");
}

///////////////////////////////////////////////////////////////////////////
// ManagedObject Implementation ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void ISPCDevice::commit()
{
  Device::commit();

  TiledLoadBalancer::instance = make_unique<LocalTiledLoadBalancer>();

  tasking::initTaskingSystem(numThreads, true);

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
      throw std::runtime_error("failed to initialize Embree");
    }
  }

  if (!vklDriver) {
    vklLoadModule("ispc_driver");

    VKLDriver driver = nullptr;

    int ispc_width = ispc::ISPCDevice_programCount();
    switch (ispc_width) {
    case 4:
      driver = vklNewDriver("ispc_4");
      break;
    case 8:
      driver = vklNewDriver("ispc_8");
      break;
    case 16:
      driver = vklNewDriver("ispc_16");
      break;
    default:
      driver = vklNewDriver("ispc");
      break;
    }

    vklDriverSetErrorFunc(driver, [](VKLError, const char *message) {
      handleError(OSP_UNKNOWN_ERROR, message);
    });

    vklDriverSetLogFunc(driver,
        [](const char *message) { postStatusMsg(OSP_LOG_INFO) << message; });

    vklDriverSetInt(driver, "logLevel", logLevel);
    vklDriverSetInt(driver, "numThreads", numThreads);

    vklCommitDriver(driver);
    vklSetCurrentDriver(driver);

    vklDriver = driver;
  }
}

///////////////////////////////////////////////////////////////////////////
// Device Implementation //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

int ISPCDevice::loadModule(const char *name)
{
  return loadLocalModule(name);
}

///////////////////////////////////////////////////////////////////////////
// OSPRay Data Arrays /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPData ISPCDevice::newSharedData(const void *sharedData,
    OSPDataType type,
    const vec3ul &numItems,
    const vec3l &byteStride)
{
  return (OSPData) new Data(sharedData, type, numItems, byteStride);
}

OSPData ISPCDevice::newData(OSPDataType type, const vec3ul &numItems)
{
  return (OSPData) new Data(type, numItems);
}

void ISPCDevice::copyData(
    const OSPData source, OSPData destination, const vec3ul &destinationIndex)
{
  Data *dst = (Data *)destination;
  dst->copy(*(Data *)source, destinationIndex);
}

///////////////////////////////////////////////////////////////////////////
// Renderable Objects /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPLight ISPCDevice::newLight(const char *type)
{
  return (OSPLight)Light::createInstance(type);
}

OSPCamera ISPCDevice::newCamera(const char *type)
{
  return (OSPCamera)Camera::createInstance(type);
}

OSPGeometry ISPCDevice::newGeometry(const char *type)
{
  return (OSPGeometry)Geometry::createInstance(type);
}

OSPVolume ISPCDevice::newVolume(const char *type)
{
  return (OSPVolume) new Volume(type);
}

OSPGeometricModel ISPCDevice::newGeometricModel(OSPGeometry _geom)
{
  auto *geom = (Geometry *)_geom;
  auto *model = new GeometricModel(geom);
  return (OSPGeometricModel)model;
}

OSPVolumetricModel ISPCDevice::newVolumetricModel(OSPVolume _volume)
{
  auto *volume = (Volume *)_volume;
  auto *model = new VolumetricModel(volume);
  return (OSPVolumetricModel)model;
}

///////////////////////////////////////////////////////////////////////////
// Model Meta-Data ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPMaterial ISPCDevice::newMaterial(
    const char *renderer_type, const char *material_type)
{
  return (OSPMaterial)Material::createInstance(renderer_type, material_type);
}

OSPTransferFunction ISPCDevice::newTransferFunction(const char *type)
{
  return (OSPTransferFunction)TransferFunction::createInstance(type);
}

OSPTexture ISPCDevice::newTexture(const char *type)
{
  return (OSPTexture)Texture::createInstance(type);
}

///////////////////////////////////////////////////////////////////////////
// Instancing /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPGroup ISPCDevice::newGroup()
{
  return (OSPGroup) new Group;
}

OSPInstance ISPCDevice::newInstance(OSPGroup _group)
{
  auto *group = (Group *)_group;
  auto *instance = new Instance(group);
  return (OSPInstance)instance;
}

///////////////////////////////////////////////////////////////////////////
// Top-level Worlds ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPWorld ISPCDevice::newWorld()
{
  return (OSPWorld) new World;
}

box3f ISPCDevice::getBounds(OSPObject _obj)
{
  auto *obj = (ManagedObject *)_obj;
  return obj->getBounds();
}

///////////////////////////////////////////////////////////////////////////
// Object + Parameter Lifetime Management /////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void ISPCDevice::setObjectParam(
    OSPObject object, const char *name, OSPDataType type, const void *mem)
{
  if (type == OSP_UNKNOWN)
    throw std::runtime_error("cannot set OSP_UNKNOWN parameter type");

  if (type == OSP_BYTE || type == OSP_RAW) {
    setParamOnObject(object, name, *(const byte_t *)mem);
    return;
  }

  setParamFcns[type](object, name, mem);
}

void ISPCDevice::removeObjectParam(OSPObject _object, const char *name)
{
  ManagedObject *object = (ManagedObject *)_object;
  ManagedObject *existing = object->getParam<ManagedObject *>(name, nullptr);
  if (existing)
    existing->refDec();
  object->removeParam(name);
}

void ISPCDevice::commit(OSPObject _object)
{
  ManagedObject *object = (ManagedObject *)_object;
  object->commit();
  object->checkUnused();
  object->resetAllParamQueryStatus();
}

void ISPCDevice::release(OSPObject _obj)
{
  ManagedObject *obj = (ManagedObject *)_obj;
  obj->refDec();
}

void ISPCDevice::retain(OSPObject _obj)
{
  ManagedObject *obj = (ManagedObject *)_obj;
  obj->refInc();
}

///////////////////////////////////////////////////////////////////////////
// FrameBuffer Manipulation ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPFrameBuffer ISPCDevice::frameBufferCreate(
    const vec2i &size, const OSPFrameBufferFormat mode, const uint32 channels)
{
  FrameBuffer::ColorBufferFormat colorBufferFormat = mode;

  FrameBuffer *fb = new LocalFrameBuffer(size, colorBufferFormat, channels);
  return (OSPFrameBuffer)fb;
}

OSPImageOperation ISPCDevice::newImageOp(const char *type)
{
  return (OSPImageOperation)ImageOp::createInstance(type);
}

const void *ISPCDevice::frameBufferMap(
    OSPFrameBuffer _fb, OSPFrameBufferChannel channel)
{
  LocalFrameBuffer *fb = (LocalFrameBuffer *)_fb;
  return fb->mapBuffer(channel);
}

void ISPCDevice::frameBufferUnmap(const void *mapped, OSPFrameBuffer _fb)
{
  FrameBuffer *fb = (FrameBuffer *)_fb;
  fb->unmap(mapped);
}

float ISPCDevice::getVariance(OSPFrameBuffer _fb)
{
  FrameBuffer *fb = (FrameBuffer *)_fb;
  return fb->getVariance();
}

void ISPCDevice::resetAccumulation(OSPFrameBuffer _fb)
{
  LocalFrameBuffer *fb = (LocalFrameBuffer *)_fb;
  fb->clear();
}

///////////////////////////////////////////////////////////////////////////
// Frame Rendering ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPRenderer ISPCDevice::newRenderer(const char *type)
{
  return (OSPRenderer)Renderer::createInstance(type);
}

OSPFuture ISPCDevice::renderFrame(OSPFrameBuffer _fb,
    OSPRenderer _renderer,
    OSPCamera _camera,
    OSPWorld _world)
{
  FrameBuffer *fb = (FrameBuffer *)_fb;
  Renderer *renderer = (Renderer *)_renderer;
  Camera *camera = (Camera *)_camera;
  World *world = (World *)_world;

  fb->setCompletedEvent(OSP_NONE_FINISHED);

  fb->refInc();
  renderer->refInc();
  camera->refInc();
  world->refInc();

  auto *f = new RenderTask(fb, [=]() {
    utility::CodeTimer timer;
    timer.start();
    renderer->renderFrame(fb, camera, world);
    timer.stop();

    fb->refDec();
    renderer->refDec();
    camera->refDec();
    world->refDec();

    return timer.seconds();
  });

  return (OSPFuture)f;
}

int ISPCDevice::isReady(OSPFuture _task, OSPSyncEvent event)
{
  auto *task = (Future *)_task;
  return task->isFinished(event);
}

void ISPCDevice::wait(OSPFuture _task, OSPSyncEvent event)
{
  auto *task = (Future *)_task;
  task->wait(event);
}

void ISPCDevice::cancel(OSPFuture _task)
{
  auto *task = (Future *)_task;
  return task->cancel();
}

float ISPCDevice::getProgress(OSPFuture _task)
{
  auto *task = (Future *)_task;
  return task->getProgress();
}

float ISPCDevice::getTaskDuration(OSPFuture _task)
{
  auto *task = (Future *)_task;
  return task->getTaskDuration();
}

OSPPickResult ISPCDevice::pick(OSPFrameBuffer _fb,
    OSPRenderer _renderer,
    OSPCamera _camera,
    OSPWorld _world,
    const vec2f &screenPos)
{
  FrameBuffer *fb = (FrameBuffer *)_fb;
  Renderer *renderer = (Renderer *)_renderer;
  Camera *camera = (Camera *)_camera;
  World *world = (World *)_world;
  return renderer->pick(fb, camera, world, screenPos);
}

extern "C" OSPError OSPRAY_DLLEXPORT ospray_module_init_ispc(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  auto status = moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR) {
    Device::registerType<ISPCDevice>("cpu");

    registerAllCameras();
    registerAllImageOps();
    registerAllGeometries();
    registerAllLights();
    registerAllMaterials();
    registerAllRenderers();
    registerAllTextures();
    registerAllTransferFunctions();
  }

  return status;
}

} // namespace api
} // namespace ospray
