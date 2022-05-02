// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <map>

#include "ISPCDevice.h"
#include "MPIDistributedDevice.h"
#include "camera/Camera.h"
#include "common/Data.h"
#include "common/DistributedWorld.h"
#include "common/Group.h"
#include "common/Instance.h"
#include "common/MPICommon.h"
#include "common/Profiling.h"
#include "fb/DistributedFrameBuffer.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "openvkl/openvkl.h"
#include "render/DistributedLoadBalancer.h"
#include "render/Material.h"
#include "render/ThreadedRenderTask.h"
#include "render/distributed/DistributedRaycast.h"
#include "rkcommon/tasking/tasking_system_init.h"
#include "rkcommon/utility/CodeTimer.h"
#include "rkcommon/utility/getEnvVar.h"
#include "volume/Volume.h"
#include "volume/VolumetricModel.h"
#include "volume/transferFunction/TransferFunction.h"
// ispc exports
#include "MPIDistributedDevice_ispc.h"

namespace ospray {
namespace mpi {

// Helper functions ///////////////////////////////////////////////////////

using SetParamFcn = void(std::shared_ptr<ospray::api::Device>,
    OSPObject,
    const char *,
    const void *m,
    OSPDataType);

template <typename T>
static void setParamOnObject(std::shared_ptr<ospray::api::Device> d,
    OSPObject _obj,
    const char *p,
    const T &v,
    OSPDataType t)
{
  auto *obj = lookupObject<ManagedObject>(_obj);
  d->setObjectParam((OSPObject)obj, p, t, &v);
}

#define declare_param_setter(TYPE)                                             \
  {                                                                            \
    OSPTypeFor<TYPE>::value,                                                   \
        [](std::shared_ptr<ospray::api::Device> d,                             \
            OSPObject o,                                                       \
            const char *p,                                                     \
            const void *v,                                                     \
            OSPDataType t) { setParamOnObject(d, o, p, *(TYPE *)v, t); }       \
  }

#define declare_param_setter_object(TYPE)                                      \
  {                                                                            \
    OSPTypeFor<TYPE>::value,                                                   \
        [](std::shared_ptr<ospray::api::Device> d,                             \
            OSPObject o,                                                       \
            const char *p,                                                     \
            const void *v,                                                     \
            OSPDataType t) {                                                   \
          auto *obj = lookupObject<ManagedObject>(                             \
              *reinterpret_cast<const OSPObject *>(v));                        \
          setParamOnObject(d, o, p, obj, t);                                   \
        }                                                                      \
  }

#define declare_param_setter_string(TYPE)                                      \
  {                                                                            \
    OSPTypeFor<TYPE>::value,                                                   \
        [](std::shared_ptr<ospray::api::Device> d,                             \
            OSPObject o,                                                       \
            const char *p,                                                     \
            const void *v,                                                     \
            OSPDataType t) {                                                   \
          const char *str = (const char *)v;                                   \
          setParamOnObject(d, o, p, std::string(str), t);                      \
        }                                                                      \
  }

static std::map<OSPDataType, std::function<SetParamFcn>> setParamFcns = {
    declare_param_setter(api::Device *),
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
    declare_param_setter_object(TransferFunction *),
    declare_param_setter_object(Volume *),
    declare_param_setter_object(VolumetricModel *),
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

template <typename OSPRAY_TYPE, typename API_TYPE>
inline API_TYPE createDistributedObject(const char *type, ObjectHandle handle)
{
  auto *instance = OSPRAY_TYPE::createInstance(type);
  handle.assign(instance);
  return (API_TYPE)(int64)handle;
}

// MPIDistributedDevice definitions ///////////////////////////////////////

MPIDistributedDevice::MPIDistributedDevice()
{
  internalDevice = std::make_shared<ospray::api::ISPCDevice>();
}

MPIDistributedDevice::~MPIDistributedDevice()
{
  messaging::shutdown();

  if (shouldFinalizeMPI) {
    try {
      MPI_CALL(Finalize());
    } catch (...) {
      // Silently move on if finalize fails
    }
  }
}

static void internalDeviceErrorFunc(
    void *, const OSPError code, const char *str)
{
  postStatusMsg() << "#OSPRay MPI InternalDevice: internal error " << code
                  << " : " << str;
  throw std::runtime_error(
      "OSPRay MPIInternalDevice internal error '" + std::string(str) + "'");
}

void MPIDistributedDevice::commit()
{
  Device::commit();
  // MPI Device defaults to not setting affinity
  if (threadAffinity == AUTO_DETECT) {
    threadAffinity = DEAFFINITIZE;
  }

  if (!initialized) {
    internalDevice->error_fcn = internalDeviceErrorFunc;

    int _ac = 1;

    const char *_av[] = {"ospray_mpi_distributed_device"};

    auto *setComm =
        static_cast<MPI_Comm *>(getParam<void *>("worldCommunicator", nullptr));
    shouldFinalizeMPI = mpicommon::init(&_ac, _av, setComm == nullptr);

    if (setComm)
      mpicommon::worker.setTo(*setComm);
    else
      mpicommon::worker = mpicommon::world;

    initialized = true;

    auto OSPRAY_FORCE_COMPRESSION =
        utility::getEnvVar<int>("OSPRAY_FORCE_COMPRESSION");
    // Turning on the compression past 64 ranks seems to be a good
    // balancing point for cost of compressing vs. performance gain
    auto enableCompression = OSPRAY_FORCE_COMPRESSION.value_or(
        mpicommon::workerSize() >= OSP_MPI_COMPRESSION_THRESHOLD);

    maml::init(enableCompression);
    messaging::init(mpicommon::worker);
    maml::start();
  }

  internalDevice->commit();
}

OSPFrameBuffer MPIDistributedDevice::frameBufferCreate(
    const vec2i &size, const OSPFrameBufferFormat mode, const uint32 channels)
{
  ObjectHandle handle = allocateHandle();
  auto *instance = new DistributedFrameBuffer(size, handle, mode, channels);
  handle.assign(instance);
  return (OSPFrameBuffer)(int64)handle;
}

const void *MPIDistributedDevice::frameBufferMap(
    OSPFrameBuffer _fb, OSPFrameBufferChannel channel)
{
  if (!mpicommon::IamTheMaster())
    throw std::runtime_error("Can only map framebuffer on the master!");

  auto *fb = lookupDistributedObject<FrameBuffer>(_fb);

  return fb->mapBuffer(channel);
}

void MPIDistributedDevice::frameBufferUnmap(
    const void *mapped, OSPFrameBuffer _fb)
{
  auto *fb = lookupDistributedObject<FrameBuffer>(_fb);
  fb->unmap(mapped);
}

void MPIDistributedDevice::resetAccumulation(OSPFrameBuffer _fb)
{
  auto *fb = lookupDistributedObject<FrameBuffer>(_fb);
  fb->clear();
}

OSPGroup MPIDistributedDevice::newGroup()
{
  return internalDevice->newGroup();
}

OSPInstance MPIDistributedDevice::newInstance(OSPGroup _group)
{
  return internalDevice->newInstance(_group);
}

OSPWorld MPIDistributedDevice::newWorld()
{
  ObjectHandle handle = allocateHandle();
  auto *instance = new DistributedWorld;
  instance->setDevice(
      ((ospray::api::ISPCDevice *)internalDevice.get())->getEmbreeDevice());
  handle.assign(instance);
  return (OSPWorld)(int64)(handle);
}

box3f MPIDistributedDevice::getBounds(OSPObject _obj)
{
  auto *obj = lookupObject<ManagedObject>(_obj);
  return internalDevice->getBounds((OSPObject)obj);
}

OSPData MPIDistributedDevice::newSharedData(const void *sharedData,
    OSPDataType type,
    const vec3ul &numItems,
    const vec3l &byteStride)
{
  return internalDevice->newSharedData(sharedData, type, numItems, byteStride);
}

OSPData MPIDistributedDevice::newData(OSPDataType type, const vec3ul &numItems)
{
  return internalDevice->newData(type, numItems);
}

void MPIDistributedDevice::copyData(
    const OSPData source, OSPData destination, const vec3ul &destinationIndex)
{
  internalDevice->copyData(source, destination, destinationIndex);
}

int MPIDistributedDevice::loadModule(const char *name)
{
  return loadLocalModule(name);
}

OSPImageOperation MPIDistributedDevice::newImageOp(const char *type)
{
  return internalDevice->newImageOp(type);
}

OSPRenderer MPIDistributedDevice::newRenderer(const char *type)
{
  ObjectHandle handle = allocateHandle();
  return createDistributedObject<Renderer, OSPRenderer>(type, handle);
}

OSPCamera MPIDistributedDevice::newCamera(const char *type)
{
  return internalDevice->newCamera(type);
}

OSPVolume MPIDistributedDevice::newVolume(const char *type)
{
  return internalDevice->newVolume(type);
}

OSPGeometry MPIDistributedDevice::newGeometry(const char *type)
{
  return internalDevice->newGeometry(type);
}

OSPGeometricModel MPIDistributedDevice::newGeometricModel(OSPGeometry _geom)
{
  auto *geom = lookupObject<Geometry>(_geom);
  return internalDevice->newGeometricModel((OSPGeometry)geom);
}

OSPVolumetricModel MPIDistributedDevice::newVolumetricModel(OSPVolume _vol)
{
  auto *volume = lookupObject<Volume>(_vol);
  return internalDevice->newVolumetricModel((OSPVolume)volume);
}

OSPMaterial MPIDistributedDevice::newMaterial(
    const char *unused, const char *material_type)
{
  return internalDevice->newMaterial(unused, material_type);
}

OSPTransferFunction MPIDistributedDevice::newTransferFunction(const char *type)
{
  return internalDevice->newTransferFunction(type);
}

OSPLight MPIDistributedDevice::newLight(const char *type)
{
  return internalDevice->newLight(type);
}

OSPFuture MPIDistributedDevice::renderFrame(OSPFrameBuffer _fb,
    OSPRenderer _renderer,
    OSPCamera _camera,
    OSPWorld _world)
{
  mpicommon::barrier(mpicommon::worker.comm).wait();
  auto *fb = lookupDistributedObject<FrameBuffer>(_fb);
  auto *renderer = lookupDistributedObject<Renderer>(_renderer);
  auto *camera = lookupObject<Camera>(_camera);
  auto *world = lookupObject<DistributedWorld>(_world);

  ObjectHandle handle = allocateHandle();
  auto loadBalancer = std::make_shared<DistributedLoadBalancer>();
  loadBalancer->setObjectHandle(handle);

  fb->setCompletedEvent(OSP_NONE_FINISHED);

  fb->refInc();
  renderer->refInc();
  camera->refInc();
  world->refInc();

  auto *f = new ThreadedRenderTask(fb, loadBalancer, [=]() {
#ifdef ENABLE_PROFILING
    using namespace mpicommon;
    ProfilingPoint start;
#endif
    utility::CodeTimer timer;
    timer.start();
    loadBalancer->renderFrame(fb, renderer, camera, world);
    timer.stop();
#ifdef ENABLE_PROFILING
    ProfilingPoint end;
    std::cout << "Frame took " << elapsedTimeMs(start, end)
              << "ms, CPU: " << cpuUtilization(start, end) << "%\n";
#endif

    fb->refDec();
    renderer->refDec();
    camera->refDec();
    world->refDec();

    return timer.seconds();
  });

  return (OSPFuture)f;
}

int MPIDistributedDevice::isReady(OSPFuture _task, OSPSyncEvent event)
{
  auto *task = (Future *)_task;
  return task->isFinished(event);
}

void MPIDistributedDevice::wait(OSPFuture _task, OSPSyncEvent event)
{
  auto *task = (Future *)_task;
  task->wait(event);
}

void MPIDistributedDevice::cancel(OSPFuture _task)
{
  auto *task = (Future *)_task;
  return task->cancel();
}

float MPIDistributedDevice::getProgress(OSPFuture _task)
{
  auto *task = (Future *)_task;
  return task->getProgress();
}

float MPIDistributedDevice::getTaskDuration(OSPFuture _task)
{
  return internalDevice->getTaskDuration(_task);
}

float MPIDistributedDevice::getVariance(OSPFrameBuffer _fb)
{
  auto *fb = lookupDistributedObject<FrameBuffer>(_fb);
  return internalDevice->getVariance((OSPFrameBuffer)fb);
}

void MPIDistributedDevice::setObjectParam(
    OSPObject object, const char *name, OSPDataType type, const void *mem)
{
  if (type == OSP_UNKNOWN)
    throw std::runtime_error("cannot set OSP_UNKNOWN parameter type");

  if (type == OSP_BYTE || type == OSP_RAW) {
    setParamOnObject(internalDevice, object, name, *(const byte_t *)mem, type);
    return;
  }

  setParamFcns[type](internalDevice, object, name, mem, type);
}

void MPIDistributedDevice::removeObjectParam(
    OSPObject _object, const char *name)
{
  auto *object = lookupObject<ManagedObject>(_object);
  return internalDevice->removeObjectParam((OSPObject)object, name);
}

void MPIDistributedDevice::commit(OSPObject _object)
{
  auto *object = lookupObject<ManagedObject>(_object);
  internalDevice->commit((OSPObject)object);
}

void MPIDistributedDevice::release(OSPObject _obj)
{
  if (!_obj)
    return;

  auto &handle = reinterpret_cast<ObjectHandle &>(_obj);
  auto *object = lookupObject<ManagedObject>(_obj);
  // TODO: dig some more here, it seems like I should be releasing when the
  // use count == 1, since it will be 0 after decrementing? Do we actually
  // have double-frees which are happening in the app?
  if (object->useCount() == 0 && handle.defined()) {
    handle.freeObject();
  } else {
    auto *obj = (ManagedObject *)object;
    obj->refDec();
  }
}

void MPIDistributedDevice::retain(OSPObject _obj)
{
  auto *object = lookupObject<ManagedObject>(_obj);
  object->refInc();
}

OSPTexture MPIDistributedDevice::newTexture(const char *type)
{
  return internalDevice->newTexture(type);
}

OSPPickResult MPIDistributedDevice::pick(OSPFrameBuffer _fb,
    OSPRenderer _renderer,
    OSPCamera _camera,
    OSPWorld _world,
    const vec2f &screenPos)
{
  auto *fb = lookupDistributedObject<FrameBuffer>(_fb);
  auto *renderer = lookupDistributedObject<Renderer>(_renderer);
  auto *camera = lookupObject<Camera>(_camera);
  auto *world = lookupObject<DistributedWorld>(_world);
  return renderer->pick(fb, camera, world, screenPos);
}

ObjectHandle MPIDistributedDevice::allocateHandle()
{
  mpicommon::barrier(mpicommon::worker.comm).wait();
  ObjectHandle handle = ObjectHandle::allocateLocalHandle();

  // For debugging check that all ranks did in fact allocate the same handle.
  // Typically we assume this is the case, as the app should be creating
  // distributed objects in lock-step, even if their local objects differ.
  if (logLevel == OSP_LOG_DEBUG) {
    int maxID = handle.i32.ID;
    int minID = handle.i32.ID;

    auto reduceMax = mpicommon::reduce(
        &handle.i32.ID, &maxID, 1, MPI_INT, MPI_MAX, 0, mpicommon::worker.comm);
    auto reduceMin = mpicommon::reduce(
        &handle.i32.ID, &minID, 1, MPI_INT, MPI_MIN, 0, mpicommon::worker.comm);
    reduceMax.wait();
    reduceMin.wait();

    if (maxID != minID) {
      // Log it, but this is a fatal error
      postStatusMsg(
          "Error allocating distributed handles: Ranks do not all have the same handle!");
      throw std::runtime_error(
          "Error allocating distributed handles: Ranks do not all have the same handle!");
    }
  }

  return handle;
}

} // namespace mpi
} // namespace ospray
