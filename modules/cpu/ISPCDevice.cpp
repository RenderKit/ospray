// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifdef OSPRAY_TARGET_SYCL
#include <level_zero/ze_api.h>
// ze_api and sycl level zero backend must be in this order
#include <sycl/ext/oneapi/backend/level_zero.hpp>
#endif

// ospray
#include "ISPCDevice.h"
#include "camera/Camera.h"
#include "common/Data.h"
#include "common/Group.h"
#include "common/Instance.h"
#include "common/Library.h"
#include "common/World.h"
#include "fb/ImageOp.h"
#include "fb/LocalFB.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "render/LoadBalancer.h"
#include "render/Material.h"
#ifdef OSPRAY_TARGET_SYCL
#include "render/RenderTaskSycl.h"
#else
#include "render/RenderTask.h"
#endif
#include "render/Renderer.h"
#include "texture/Texture.h"
#include "texture/Texture2D.h"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "volume/VolumetricModel.h"
#include "volume/transferFunction/TransferFunction.h"
#endif

// stl
#include <algorithm>
#include <functional>
#include <map>
#include "rkcommon/tasking/tasking_system_init.h"
#include "rkcommon/utility/CodeTimer.h"

#ifndef OSPRAY_TARGET_SYCL
#include "ISPCDevice_ispc.h"
#endif

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
    declare_param_setter_object(Geometry *),
    declare_param_setter_object(GeometricModel *),
    declare_param_setter_object(Group *),
    declare_param_setter_object(ImageOp *),
    declare_param_setter_object(Instance *),
    declare_param_setter_object(Light *),
    declare_param_setter_object(Material *),
    declare_param_setter_object(Renderer *),
    declare_param_setter_object(Texture *),
#ifdef OSPRAY_ENABLE_VOLUMES
    declare_param_setter_object(TransferFunction *),
    declare_param_setter_object(Volume *),
    declare_param_setter_object(VolumetricModel *),
#endif
    declare_param_setter_object(World *),
    declare_param_setter_string(const char *),
    declare_param_setter(char *),
    declare_param_setter(char),
    declare_param_setter(vec2c),
    declare_param_setter(vec3c),
    declare_param_setter(vec4c),
    declare_param_setter(unsigned char),
    declare_param_setter(vec2uc),
    declare_param_setter(vec3uc),
    declare_param_setter(vec4uc),
    declare_param_setter(short),
    declare_param_setter(vec2s),
    declare_param_setter(vec3s),
    declare_param_setter(vec4s),
    declare_param_setter(unsigned short),
    declare_param_setter(vec2us),
    declare_param_setter(vec3us),
    declare_param_setter(vec4us),
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
    declare_param_setter(vec2d),
    declare_param_setter(vec3d),
    declare_param_setter(vec4d),
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

ISPCDevice::ISPCDevice()
    : loadBalancer(std::make_shared<LocalTiledLoadBalancer>())
{}

ISPCDevice::~ISPCDevice()
{
  try {
    if (embreeDevice) {
      rtcReleaseDevice(embreeDevice);
    }
  } catch (...) {
    // silently move on, sometimes a pthread mutex lock fails in Embree
  }

#ifdef OSPRAY_ENABLE_VOLUMES
  if (vklDevice) {
    vklReleaseDevice(vklDevice);
  }
#endif
}

static void embreeErrorFunc(void *, const RTCError code, const char *str)
{
  postStatusMsg() << "#osp: Embree internal error " << code << " : " << str;
  OSPError e =
      (code > RTC_ERROR_UNSUPPORTED_CPU) ? OSP_UNKNOWN_ERROR : (OSPError)code;
  handleError(e, "Embree internal error '" + std::string(str) + "'");
}

#ifdef OSPRAY_ENABLE_VOLUMES
static void vklErrorFunc(void *, const VKLError code, const char *str)
{
  postStatusMsg() << "#osp: Open VKL internal error " << code << " : " << str;
  OSPError e =
      (code > VKL_UNSUPPORTED_CPU) ? OSP_UNKNOWN_ERROR : (OSPError)code;
  handleError(e, "Open VKL internal error '" + std::string(str) + "'");
}
#endif

void ISPCDevice::commit()
{
  Device::commit();

  // TODO: Should this somehow report an error if app trying to change and
  // recommit these params?
  if (!ispcrtContext) {
#ifdef OSPRAY_TARGET_SYCL
    sycl::context *appSyclCtx =
        static_cast<sycl::context *>(getParam<void *>("syclContext", nullptr));
    sycl::device *appSyclDevice =
        static_cast<sycl::device *>(getParam<void *>("syclDevice", nullptr));
    if ((appSyclCtx && !appSyclDevice) || (!appSyclCtx && appSyclDevice)) {
      handleError(OSP_INVALID_OPERATION,
          "OSPRay ISPCDevice invalid configuration: if providing a syclContext and syclDevice both must be provided");
      return;
    }

    ze_context_handle_t *appZeCtx = static_cast<ze_context_handle_t *>(
        getParam<void *>("zeContext", nullptr));
    ze_device_handle_t *appZeDevice = static_cast<ze_device_handle_t *>(
        getParam<void *>("zeDevice", nullptr));
    if ((appZeCtx && !appZeDevice) || (!appZeCtx && appZeDevice)) {
      handleError(OSP_INVALID_OPERATION,
          "OSPRay ISPCDevice invalid configuration: if providing a zeContext and zeDevice both must be provided");
      return;
    }

    if (appSyclCtx && appZeCtx) {
      handleError(OSP_INVALID_OPERATION,
          "OSPRay ISPCDevice invalid configuration: For SYCL or Level Zero context interopability only a SYCL or Level Zero context & device can be provided, not both.");
      return;
    }

    // If we got a SYCL context just get the native handles and set the "app" L0
    // context/device to them, since we can take the same code path from there
    ze_context_handle_t syclZeCtxHandle;
    ze_device_handle_t syclZeDeviceHandle;
    if (appSyclCtx) {
      syclZeCtxHandle =
          sycl::get_native<sycl::backend::ext_oneapi_level_zero>(*appSyclCtx);
      syclZeDeviceHandle =
          sycl::get_native<sycl::backend::ext_oneapi_level_zero>(
              *appSyclDevice);

      appZeCtx = &syclZeCtxHandle;
      appZeDevice = &syclZeDeviceHandle;
    }

    if (appZeCtx) {
      ispcrtContext = ispcrt::Context(
          ISPCRT_DEVICE_TYPE_GPU, (ISPCRTGenericHandle)*appZeCtx);
      ispcrtDevice =
          ispcrt::Device(ispcrtContext, (ISPCRTGenericHandle)*appZeDevice);
    } else {
      ispcrt::Context *ispcrtContextPtr = static_cast<ispcrt::Context *>(
          getParam<void *>("ispcrtContext", nullptr));
      ispcrt::Device *ispcrtDevicePtr = static_cast<ispcrt::Device *>(
          getParam<void *>("ispcrtDevice", nullptr));

      if (ispcrtContextPtr != nullptr && ispcrtDevicePtr != nullptr) {
        ispcrtContext = *ispcrtContextPtr;
        ispcrtDevice = *ispcrtDevicePtr;
      } else {
        ispcrtContext = ispcrt::Context(ISPCRT_DEVICE_TYPE_GPU);
        ispcrtDevice = ispcrt::Device(ispcrtContext);
      }
    }
#else
    ispcrtContext = ispcrt::Context(ISPCRT_DEVICE_TYPE_CPU);
    ispcrtDevice = ispcrt::Device(ispcrtContext);
#endif
    ispcrtQueue = ispcrt::TaskQueue(ispcrtDevice);

#ifdef OSPRAY_TARGET_SYCL
    syclPlatform = sycl::ext::oneapi::level_zero::make_platform(
        reinterpret_cast<pi_native_handle>(
            ispcrtDevice.nativePlatformHandle()));
    syclDevice = sycl::ext::oneapi::level_zero::make_device(syclPlatform,
        reinterpret_cast<pi_native_handle>(ispcrtDevice.nativeDeviceHandle()));

    syclContext = sycl::ext::oneapi::level_zero::make_context(
        std::vector<sycl::device>{syclDevice},
        reinterpret_cast<pi_native_handle>(ispcrtDevice.nativeContextHandle()),
        true);

    syclQueue = sycl::queue(syclContext,
        syclDevice,
        {sycl::property::queue::enable_profiling(),
            sycl::property::queue::in_order()});
#endif
  }

  tasking::initTaskingSystem(numThreads, true);

  if (!embreeDevice) {
#ifdef OSPRAY_TARGET_SYCL
    embreeDevice =
        rtcNewSYCLDevice(syclContext, generateEmbreeDeviceCfg(*this).c_str());
    rtcSetDeviceSYCLDevice(embreeDevice, syclDevice);
#else
    embreeDevice = rtcNewDevice(generateEmbreeDeviceCfg(*this).c_str());
#endif
    rtcSetDeviceErrorFunction(embreeDevice, embreeErrorFunc, nullptr);
    RTCError erc = rtcGetDeviceError(embreeDevice);
    if (erc != RTC_ERROR_NONE) {
      // why did the error function not get called !?
      postStatusMsg() << "#osp:init: embree internal error number " << erc;
      throw std::runtime_error("failed to initialize Embree");
    }
  }

#ifdef OSPRAY_ENABLE_VOLUMES
  if (!vklDevice) {
#if OPENVKL_VERSION_MAJOR == 1
    vklLoadModule("cpu_device");
#else
    vklInit();
#endif

#ifdef OSPRAY_TARGET_SYCL
    vklDevice = vklNewDevice("gpu_4");
    vklDeviceSetVoidPtr(
        vklDevice, "syclContext", static_cast<void *>(&syclContext));
#else
    int cpu_width = ispc::ISPCDevice_programCount();
    switch (cpu_width) {
    case 4:
      vklDevice = vklNewDevice("cpu_4");
      break;
    case 8:
      vklDevice = vklNewDevice("cpu_8");
      break;
    case 16:
      vklDevice = vklNewDevice("cpu_16");
      break;
    default:
      vklDevice = vklNewDevice("cpu");
      break;
    }
#endif

    vklDeviceSetErrorCallback(vklDevice, vklErrorFunc, nullptr);
    vklDeviceSetLogCallback(
        vklDevice,
        [](void *, const char *message) {
          postStatusMsg(OSP_LOG_INFO) << message;
        },
        nullptr);

    vklDeviceSetInt(vklDevice, "logLevel", logLevel);
    vklDeviceSetInt(vklDevice, "numThreads", numThreads);

    vklCommitDevice(vklDevice);
  }
#endif

#ifndef OSPRAY_TARGET_SYCL
  // Output device info string
  const char *isaNames[] = {
      "unknown", "SSE2", "SSE4", "AVX", "AVX2", "AVX512SKX", "NEON"};
  postStatusMsg(OSP_LOG_INFO)
      << "Using ISPC device with " << isaNames[ispc::ISPCDevice_isa()]
      << " instruction set";
#else
  postStatusMsg(OSP_LOG_INFO)
      << "Using SYCL GPU device on "
      << syclDevice.get_info<sycl::info::device::name>() << " device";
#endif
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
  return (OSPData) new Data(*this, sharedData, type, numItems, byteStride);
}

OSPData ISPCDevice::newData(OSPDataType type, const vec3ul &numItems)
{
  return (OSPData) new Data(*this, type, numItems);
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
  return (OSPLight)Light::createInstance(type, *this);
}

OSPCamera ISPCDevice::newCamera(const char *type)
{
  return (OSPCamera)Camera::createInstance(type, *this);
}

OSPGeometry ISPCDevice::newGeometry(const char *type)
{
  return (OSPGeometry)Geometry::createInstance(type, *this);
}

OSPVolume ISPCDevice::newVolume(const char *type)
{
#ifdef OSPRAY_ENABLE_VOLUMES
  return (OSPVolume) new Volume(*this, type);
#else
  return (OSPVolume) nullptr;
#endif
}

OSPGeometricModel ISPCDevice::newGeometricModel(OSPGeometry _geom)
{
  auto *geom = (Geometry *)_geom;
  auto *model = new GeometricModel(*this, geom);
  return (OSPGeometricModel)model;
}

OSPVolumetricModel ISPCDevice::newVolumetricModel(OSPVolume _volume)
{
#ifdef OSPRAY_ENABLE_VOLUMES
  auto *volume = (Volume *)_volume;
  auto *model = new VolumetricModel(*this, volume);
  return (OSPVolumetricModel)model;
#else
  return (OSPVolumetricModel) nullptr;
#endif
}

///////////////////////////////////////////////////////////////////////////
// Model Meta-Data ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPMaterial ISPCDevice::newMaterial(
    const char * /*renderer_type - unused*/, const char *material_type)
{
  return (OSPMaterial)Material::createInstance(material_type, *this);
}

OSPTransferFunction ISPCDevice::newTransferFunction(const char *type)
{
#ifdef OSPRAY_ENABLE_VOLUMES
  return (OSPTransferFunction)TransferFunction::createInstance(type, *this);
#else
  return (OSPTransferFunction) nullptr;
#endif
}

OSPTexture ISPCDevice::newTexture(const char *type)
{
  return (OSPTexture)Texture::createInstance(type, *this);
}

///////////////////////////////////////////////////////////////////////////
// Instancing /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPGroup ISPCDevice::newGroup()
{
  return (OSPGroup) new Group(*this);
}

OSPInstance ISPCDevice::newInstance(OSPGroup _group)
{
  auto *group = (Group *)_group;
  auto *instance = new Instance(*this, group);
  return (OSPInstance)instance;
}

///////////////////////////////////////////////////////////////////////////
// Top-level Worlds ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPWorld ISPCDevice::newWorld()
{
  return (OSPWorld) new World(*this);
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

  FrameBuffer *fb =
      new LocalFrameBuffer(*this, size, colorBufferFormat, channels);
  return (OSPFrameBuffer)fb;
}

OSPImageOperation ISPCDevice::newImageOp(const char *type)
{
  ospray::ImageOp *ret = ImageOp::createImageOp(type, *this);
  return (OSPImageOperation)ret;
}

const void *ISPCDevice::frameBufferMap(
    OSPFrameBuffer _fb, OSPFrameBufferChannel channel)
{
  FrameBuffer *fb = (FrameBuffer *)_fb;
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
  FrameBuffer *fb = (FrameBuffer *)_fb;
  fb->clear();
}

///////////////////////////////////////////////////////////////////////////
// Frame Rendering ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPRenderer ISPCDevice::newRenderer(const char *type)
{
  return (OSPRenderer)Renderer::createInstance(type, *this);
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

#ifndef OSPRAY_TARGET_SYCL
  fb->setCompletedEvent(OSP_NONE_FINISHED);

  fb->refInc();
  renderer->refInc();
  camera->refInc();
  world->refInc();

  return (OSPFuture) new RenderTask(fb, [=]() {
    utility::CodeTimer timer;
    timer.start();
    loadBalancer->renderFrame(fb, renderer, camera, world);
    timer.stop();

    fb->refDec();
    renderer->refDec();
    camera->refDec();
    world->refDec();

    return timer.seconds();
  });
#else
  std::pair<AsyncEvent, AsyncEvent> events =
      loadBalancer->renderFrame(fb, renderer, camera, world, false);
  return (OSPFuture) new RenderTask(events.first, events.second);
#endif
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

#ifdef OSPRAY_TARGET_SYCL
sycl::nd_range<1> ISPCDevice::computeDispatchRange(
    const size_t globalSize, const size_t workgroupSize) const
{
  // roundedRange global size must be at least workgroupSize
  const size_t roundedRange =
      std::max(size_t(1), (globalSize + workgroupSize - 1) / workgroupSize)
      * workgroupSize;
  return sycl::nd_range<1>(roundedRange, workgroupSize);
}
#endif

} // namespace api
} // namespace ospray
