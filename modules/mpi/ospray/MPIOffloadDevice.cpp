// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifndef _WIN32
#include <dlfcn.h>
#include <unistd.h>
#endif

#ifdef OPEN_MPI
#include <sched.h>
#include <thread>
#endif

#include "MPIOffloadDevice.h"
#include "camera/Camera.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/MPIBcastFabric.h"
#include "common/MPICommon.h"
#include "common/OSPWork.h"
#include "common/SocketBcastFabric.h"
#include "common/Util.h"
#include "common/World.h"
#include "fb/DistributedFrameBuffer.h"
#include "fb/LocalFB.h"
#include "render/DistributedLoadBalancer.h"
#include "render/RenderTask.h"
#include "render/Renderer.h"
#include "rkcommon/networking/DataStreaming.h"
#include "rkcommon/networking/Socket.h"
#include "rkcommon/utility/ArrayView.h"
#include "rkcommon/utility/FixedArrayView.h"
#include "rkcommon/utility/OwnedArray.h"
#include "rkcommon/utility/getEnvVar.h"
#include "volume/Volume.h"

namespace ospray {
namespace mpi {

using namespace mpicommon;
using namespace rkcommon;

///////////////////////////////////////////////////////////////////////////
// Forward declarations ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//! this runs an ospray worker process.
/*! it's up to the proper init routine to decide which processes
  call this function and which ones don't. This function will not
  return. */
void runWorker(bool useMPIFabric);

///////////////////////////////////////////////////////////////////////////
// Misc helper functions //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static inline void throwIfNotMpiParallel()
{
  if (world.size <= 1) {
    throw std::runtime_error(
        "No MPI workers found.\n#osp:mpi: Fatal Error "
        "- OSPRay told to run in MPI mode, but there "
        "seems to be no MPI peers!?\n#osp:mpi: (Did "
        "you forget an 'mpirun' in front of your "
        "application?)");
  }
}

static inline void setupMaster()
{
  MPI_Comm appComm;
  MPI_CALL(Comm_split(world.comm, 1, world.rank, &appComm));

  int size = 0;
  int rank = 0;
  MPI_Comm_size(appComm, &size);
  MPI_Comm_rank(appComm, &rank);

  postStatusMsg(OSP_LOG_INFO)
      << "#w: app process " << rank << '/' << size << " (global " << world.rank
      << '/' << world.size << ')';

  MPI_CALL(Barrier(world.comm));

  // -------------------------------------------------------
  // at this point, all processes should be set up and synced. in
  // particular:
  // - app has intracommunicator to all workers (and vica versa)
  // - app process(es) are in one intercomm ("app"); workers all in
  //   another ("worker")
  // - all processes (incl app) have barrier'ed, and thus now in sync.
}

static inline void setupWorker()
{
  MPI_CALL(Comm_split(world.comm, 0, world.rank, &worker.comm));

  worker.makeIntraComm();

  postStatusMsg(OSP_LOG_INFO)
      << "master: Made 'worker' intercomm (through split): " << std::hex
      << std::showbase << worker.comm << std::noshowbase << std::dec;

  MPI_CALL(Barrier(world.comm));

  // -------------------------------------------------------
  // at this point, all processes should be set up and synced. in
  // particular:
  // - app has intracommunicator to all workers (and vica versa)
  // - app process(es) are in one intercomm ("app"); workers all in
  //   another ("worker")
  // - all processes (incl app) have barrier'ed, and thus now in sync.
}

///////////////////////////////////////////////////////////////////////////
// MPI initialization helper functions ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

/*! TODO WILL: Update these comments
 in this mode ("ospray on ranks" mode, or "ranks" mode), the
    user has launched the app across all ranks using mpirun "<all
    rank> <app>"; no new processes need to get launched.

    Based on the 'startworkers' flag, this function can set up ospray in
    one of two modes:

    in "workers" mode (startworkers=true) all ranks > 0 become
    workers, and will NOT return to the application; rank 0 is the
    master that controls those workers but doesn't do any
    rendeirng (we may at some point allow the master to join in
    working as well, but currently this is not implemented). to
    reach that mode we call this function with
    'startworkers=true', which will make sure that, even though
    all ranks _called_ mpiinit, only rank 0 will ever return to
    the app, while all other ranks will automatically go to
    running worker code, and never ever return from this function.

    b) in "distributed" mode the app itself is distributed, and
    will use the ospray distributed api to control ospray in a
    data-distributed mode. in this way, we'll call this function
    with startWorkers=false, which will let all ranks return from
    this function to do further work in the app.

    For this function, we assume:

    - all *all* MPI_COMM_WORLD processes are going into this function

    - this fct is called from ospInit (with ranksBecomeWorkers=true) or
      from ospdMpiInit (w/ ranksBecomeWorkers = false)
*/
void createMPI_RanksBecomeWorkers(int *ac, const char **av)
{
  mpi::init(ac, av, true);

  postStatusMsg(OSP_LOG_INFO)
      << "#o: initMPI::OSPonRanks: " << world.rank << '/' << world.size;

  // Note: here we don't run this collective through MAML, since we
  // haven't started it yet.
  MPI_CALL(Barrier(world.comm));

  throwIfNotMpiParallel();

  if (world.rank == 0)
    setupMaster();
  else {
    setupWorker();

    // now, all workers will enter their worker loop (ie, they will *not*
    // return)
    mpi::runWorker(true);
    throw std::runtime_error("should never reach here!");
    /* no return here - 'runWorker' will never return */
  }
}

void createMPI_ListenForClient(int *ac, const char **av)
{
  mpi::init(ac, av, true);

  if (world.rank == 0) {
    postStatusMsg(OSP_LOG_INFO)
        << "#o: Initialize OSPRay MPI in 'Listen for Client' Mode";
  }
  worker.comm = world.comm;
  worker.makeIntraComm();

  mpi::runWorker(false);
}

///////////////////////////////////////////////////////////////////////////
// MPIDevice definitions //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

MPIOffloadDevice::~MPIOffloadDevice()
{
  // TODO: This test needs to be corrected for handlign offload w/ sockets
  // in connect/accept mode.
  if (dynamic_cast<MPIFabric *>(fabric.get()) && world.rank == 0) {
    postStatusMsg(OSP_LOG_INFO) << "shutting down mpi device";

#ifdef ENABLE_PROFILING
    {
      ProfilingPoint masterEnd;
      char hostname[512] = {0};
      gethostname(hostname, 511);
      const std::string master_log_file = std::string(hostname) + "_master.txt";
      std::ofstream fout(master_log_file.c_str(), std::ios::app);
      fout << "Shutting down, final /proc/self/status:\n"
           << getProcStatus() << "\n=====\n"
           << "Avg. CPU % during run: "
           << cpuUtilization(masterStart, masterEnd) << "%\n";
    }
#endif

    networking::BufferWriter writer;
    writer << work::FINALIZE;
    sendWork(writer.buffer, true);

    // TODO: if not mpi, don't finalize on head node
    MPI_Finalize();
  }
}

void MPIOffloadDevice::initializeDevice()
{
  Device::commit();
  // MPI Device defaults to not setting affinity
  if (threadAffinity == AUTO_DETECT) {
    threadAffinity = DEAFFINITIZE;
  }

  initialized = true;

  int _ac = 2;
  const char *_av[] = {"ospray_mpi_worker", "--osp:mpi"};

  std::string mode = getParam<std::string>("mpiMode", "mpi");

  if (mode == "mpi") {
    createMPI_RanksBecomeWorkers(&_ac, _av);

#ifdef ENABLE_PROFILING
    char hostname[512] = {0};
    gethostname(hostname, 511);
    const std::string master_log_file = std::string(hostname) + "_master.txt";
    std::ofstream fout(master_log_file.c_str());
    fout << "Master on '" << hostname << "' before commit\n"
         << "/proc/self/status:\n"
         << getProcStatus() << "\n=====\n";
    masterStart = ProfilingPoint();
#endif

    // Only the master returns from this call
    fabric = rkcommon::make_unique<MPIFabric>(world, 0);
    maml::init(false);
    maml::start();
  } else if (mode == "mpi-listen") {
    createMPI_ListenForClient(&_ac, _av);
  } else if (mode == "mpi-connect") {
    const std::string host = getParam<std::string>("host", "");
    const std::string portParam = getParam<std::string>("port", "");

    if (host.empty() || portParam.empty()) {
      throw std::runtime_error(
          "Error: mpi-connect requires a host and port "
          "argument to connect to");
    }
    int port = std::stoi(portParam);
    postStatusMsg(OSP_LOG_INFO)
        << "MPIOffloadDevice connecting to " << host << ":" << port;
    fabric = rkcommon::make_unique<SocketWriterFabric>(host, port);
  } else {
    throw std::runtime_error("Invalid MPI mode!");
  }

  // Setup the command buffer on the app rank
  // TODO: Decide on good defaults
  maxBufferedCommands = getParam<uint32_t>("maxBufferedCommands", 8192);
  commandBufferSize =
      getParam<uint32_t>("commandBufferSize", 512 * 1024 * 1024);
  maxInlineDataSize = getParam<uint32_t>("maxInlineDataSize", 8 * 1024 * 1024);

  auto OSPRAY_MPI_MAX_BUFFERED_CMDS =
      utility::getEnvVar<int>("OSPRAY_MPI_MAX_BUFFERED_CMDS");
  if (OSPRAY_MPI_MAX_BUFFERED_CMDS) {
    maxBufferedCommands = OSPRAY_MPI_MAX_BUFFERED_CMDS.value();
  }
  auto OSPRAY_MPI_CMD_BUFFER_SIZE =
      utility::getEnvVar<int>("OSPRAY_MPI_CMD_BUFFER_SIZE");
  if (OSPRAY_MPI_CMD_BUFFER_SIZE) {
    commandBufferSize = OSPRAY_MPI_CMD_BUFFER_SIZE.value();
  }
  auto OSPRAY_MPI_CMD_BUFFER_INLINE_DATA_SIZE =
      utility::getEnvVar<int>("OSPRAY_MPI_CMD_BUFFER_INLINE_DATA_SIZE");
  if (OSPRAY_MPI_CMD_BUFFER_INLINE_DATA_SIZE) {
    maxInlineDataSize = OSPRAY_MPI_CMD_BUFFER_INLINE_DATA_SIZE.value();
  }

  if (commandBufferSize >= 2e9) {
    static WarnOnce warn(
        "Command buffer size must be less than 2GB, resetting to 1.8GB");
    commandBufferSize = 1.8e9;
  }
  if (maxInlineDataSize >= commandBufferSize) {
    static WarnOnce warn(
        "Max inline data size must be less than command buffer size");
    maxInlineDataSize = std::ceil(commandBufferSize / 2.f);
  }

  commandBuffer =
      std::make_shared<utility::FixedArray<uint8_t>>(commandBufferSize);
}

///////////////////////////////////////////////////////////////////////////
// ManagedObject Implementation ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void MPIOffloadDevice::commit()
{
  if (!initialized)
    initializeDevice();
}

///////////////////////////////////////////////////////////////////////////
// Device Implementation //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

int MPIOffloadDevice::loadModule(const char *name)
{
  networking::BufferWriter writer;
  writer << work::LOAD_MODULE << std::string(name);
  sendWork(writer.buffer, false);

  // TODO: Error reporting from the workers
  return 0;
}

///////////////////////////////////////////////////////////////////////////
// Renderable Objects /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPLight MPIOffloadDevice::newLight(const char *type)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_LIGHT << handle.i64 << std::string(type);
  sendWork(writer.buffer, false);

  return (OSPLight)(int64)handle;
}

OSPCamera MPIOffloadDevice::newCamera(const char *type)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_CAMERA << handle.i64 << std::string(type);
  sendWork(writer.buffer, false);

  return (OSPCamera)(int64)handle;
}

OSPGeometry MPIOffloadDevice::newGeometry(const char *type)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_GEOMETRY << handle.i64 << std::string(type);
  sendWork(writer.buffer, false);

  return (OSPGeometry)(int64)handle;
}

OSPVolume MPIOffloadDevice::newVolume(const char *type)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_VOLUME << handle.i64 << std::string(type);
  sendWork(writer.buffer, false);

  return (OSPVolume)(int64)handle;
}

OSPGeometricModel MPIOffloadDevice::newGeometricModel(OSPGeometry geom)
{
  ObjectHandle handle = allocateHandle();

  ObjectHandle geomHandle = (ObjectHandle &)geom;
  networking::BufferWriter writer;
  writer << work::NEW_GEOMETRIC_MODEL << handle.i64 << geomHandle.i64;
  sendWork(writer.buffer, false);

  return (OSPGeometricModel)(int64)handle;
}

OSPVolumetricModel MPIOffloadDevice::newVolumetricModel(OSPVolume volume)
{
  ObjectHandle handle = allocateHandle();

  ObjectHandle volHandle = (ObjectHandle &)volume;
  networking::BufferWriter writer;
  writer << work::NEW_VOLUMETRIC_MODEL << handle.i64 << volHandle.i64;
  sendWork(writer.buffer, false);

  return (OSPVolumetricModel)(int64)handle;
}

///////////////////////////////////////////////////////////////////////////
// Model Meta-Data ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPMaterial MPIOffloadDevice::newMaterial(
    const char *renderer_type, const char *material_type)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_MATERIAL << handle.i64 << std::string(renderer_type)
         << std::string(material_type);
  sendWork(writer.buffer, false);

  return (OSPMaterial)(int64)handle;
}

OSPTransferFunction MPIOffloadDevice::newTransferFunction(const char *type)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_TRANSFER_FUNCTION << handle.i64 << std::string(type);
  sendWork(writer.buffer, false);

  return (OSPTransferFunction)(int64)handle;
}

OSPTexture MPIOffloadDevice::newTexture(const char *type)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_TEXTURE << handle.i64 << std::string(type);
  sendWork(writer.buffer, false);

  return (OSPTexture)(int64)handle;
}

///////////////////////////////////////////////////////////////////////////
// Instancing /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPGroup MPIOffloadDevice::newGroup()
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_GROUP << handle.i64;
  sendWork(writer.buffer, false);

  return (OSPGroup)(int64)handle;
}

OSPInstance MPIOffloadDevice::newInstance(OSPGroup group)
{
  ObjectHandle handle = allocateHandle();
  ObjectHandle groupHandle = (ObjectHandle &)group;

  networking::BufferWriter writer;
  writer << work::NEW_INSTANCE << handle.i64 << groupHandle.i64;
  sendWork(writer.buffer, false);

  return (OSPInstance)(int64)handle;
}

///////////////////////////////////////////////////////////////////////////
// World Manipulation /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPWorld MPIOffloadDevice::newWorld()
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_WORLD << handle.i64;
  sendWork(writer.buffer, false);

  return (OSPWorld)(int64)handle;
}

box3f MPIOffloadDevice::getBounds(OSPObject _obj)
{
  const ObjectHandle obj = (ObjectHandle &)_obj;

  networking::BufferWriter writer;
  writer << work::GET_BOUNDS << obj;
  sendWork(writer.buffer, true);

  box3f result;
  utility::ArrayView<uint8_t> view(
      reinterpret_cast<uint8_t *>(&result), sizeof(box3f));
  fabric->recv(view, rootWorkerRank());
  return result;
}

///////////////////////////////////////////////////////////////////////////
// OSPRay Data Arrays /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPData MPIOffloadDevice::newSharedData(const void *sharedData,
    OSPDataType format,
    const vec3ul &numItems,
    const vec3l &byteStride)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_SHARED_DATA << handle.i64 << format << numItems
         << byteStride;

  vec3l stride = byteStride;
  if (stride.x == 0) {
    stride.x = sizeOf(format);
  }
  if (stride.y == 0) {
    stride.y = numItems.x * sizeOf(format);
  }
  if (stride.z == 0) {
    stride.z = numItems.x * numItems.y * sizeOf(format);
  }

  uint64_t nbytes = numItems.x * stride.x;
  if (numItems.y > 1) {
    nbytes = numItems.y * stride.y;
  }
  if (numItems.z > 1) {
    nbytes = numItems.z * stride.z;
  }

  // TODO For message batching: check what the size of the incoming data is,
  // if it plus our current buffered size is > 512MB (or some env-set threshold)
  // we can send out the buffer asynchronously. We do want renderframe to
  // be send the current buffer as well to get rendering started as quickly
  // as possible.
  //
  // So we have a split between what a "flushing" operation is and a buffer
  // sending operation. Since the buffered up set of commands/data we can
  // send asynchronously and open a new command buffer to fill up without
  // having to block the application.
  //
  // The Worker's new data command will need a flag to tell if the data is
  // embedded or coming separately. For data being set > threshold we'll send
  // it as we do now, through a separate send and then have to mark that
  // we have a release hazard on that object. If that handle is released we
  // need to flush the messages out.

  auto dataView = std::make_shared<utility::ArrayView<uint8_t>>(
      const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(sharedData)),
      nbytes);

  this->sharedData[handle.i64] = dataView;
  sendDataWork(writer, dataView);

  return (OSPData)(int64)handle;
}

OSPData MPIOffloadDevice::newData(OSPDataType format, const vec3ul &numItems)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_DATA << handle.i64 << format << numItems;
  sendWork(writer.buffer, false);

  return (OSPData)(int64)handle;
}

void MPIOffloadDevice::copyData(
    const OSPData source, OSPData destination, const vec3ul &destinationIndex)
{
  networking::BufferWriter writer;
  const ObjectHandle sourceHandle = (const ObjectHandle &)source;
  ObjectHandle destinationHandle = (ObjectHandle &)destination;
  writer << work::COPY_DATA << sourceHandle.i64 << destinationHandle.i64
         << destinationIndex;
  sendWork(writer.buffer, false);
}

///////////////////////////////////////////////////////////////////////////
// Object + Parameter Lifetime Management /////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void MPIOffloadDevice::setObjectParam(
    OSPObject object, const char *name, OSPDataType type, const void *mem)
{
  ObjectHandle handle = (ObjectHandle &)object;
  switch (type) {
  // All OSP_OBJECT fall through the same style of setting param since it's just
  // a handle
  case OSP_DEVICE:
  case OSP_OBJECT:
  case OSP_CAMERA:
  case OSP_DATA:
  case OSP_FRAMEBUFFER:
  case OSP_FUTURE:
  case OSP_GEOMETRIC_MODEL:
  case OSP_GEOMETRY:
  case OSP_GROUP:
  case OSP_IMAGE_OPERATION:
  case OSP_INSTANCE:
  case OSP_LIGHT:
  case OSP_MATERIAL:
  case OSP_RENDERER:
  case OSP_TEXTURE:
  case OSP_TRANSFER_FUNCTION:
  case OSP_VOLUME:
  case OSP_VOLUMETRIC_MODEL:
  case OSP_WORLD:
    setParam<ObjectHandle>(handle, name, mem, type);
    break;
  case OSP_BOOL:
    setParam<bool>(handle, name, mem, type);
    break;
  case OSP_STRING: {
    networking::BufferWriter writer;
    writer << work::SET_PARAM << handle.i64 << std::string(name) << type
           << (const char *)mem;
    sendWork(writer.buffer, false);
    break;
  }
  case OSP_CHAR:
  case OSP_BYTE:
    setParam<char>(handle, name, mem, type);
    break;
  case OSP_VEC2UC:
    setParam<vec2uc>(handle, name, mem, type);
    break;
  case OSP_VEC3UC:
    setParam<vec3uc>(handle, name, mem, type);
    break;
  case OSP_VEC4UC:
    setParam<vec4uc>(handle, name, mem, type);
    break;
  case OSP_SHORT:
    setParam<int16_t>(handle, name, mem, type);
    break;
    // Will note: looks like ushort is missing in ispcdevice
  case OSP_USHORT:
    setParam<uint16_t>(handle, name, mem, type);
    break;
  case OSP_INT:
    setParam<int>(handle, name, mem, type);
    break;
  case OSP_VEC2I:
    setParam<vec2i>(handle, name, mem, type);
    break;
  case OSP_VEC3I:
    setParam<vec3i>(handle, name, mem, type);
    break;
  case OSP_VEC4I:
    setParam<vec4i>(handle, name, mem, type);
    break;
  case OSP_UINT:
    setParam<unsigned int>(handle, name, mem, type);
    break;
  case OSP_VEC2UI:
    setParam<vec2ui>(handle, name, mem, type);
    break;
  case OSP_VEC3UI:
    setParam<vec3ui>(handle, name, mem, type);
    break;
  case OSP_VEC4UI:
    setParam<vec4ui>(handle, name, mem, type);
    break;
  case OSP_LONG:
    setParam<long>(handle, name, mem, type);
    break;
  case OSP_VEC2L:
    setParam<vec2l>(handle, name, mem, type);
    break;
  case OSP_VEC3L:
    setParam<vec3l>(handle, name, mem, type);
    break;
  case OSP_VEC4L:
    setParam<vec4l>(handle, name, mem, type);
    break;
  case OSP_ULONG:
    setParam<unsigned long>(handle, name, mem, type);
    break;
  case OSP_VEC2UL:
    setParam<vec2ul>(handle, name, mem, type);
    break;
  case OSP_VEC3UL:
    setParam<vec3ul>(handle, name, mem, type);
    break;
  case OSP_VEC4UL:
    setParam<vec4ul>(handle, name, mem, type);
    break;
  case OSP_FLOAT:
    setParam<float>(handle, name, mem, type);
    break;
  case OSP_VEC2F:
    setParam<vec2f>(handle, name, mem, type);
    break;
  case OSP_VEC3F:
    setParam<vec3f>(handle, name, mem, type);
    break;
  case OSP_VEC4F:
    setParam<vec4f>(handle, name, mem, type);
    break;
  case OSP_DOUBLE:
    setParam<double>(handle, name, mem, type);
    break;
  case OSP_BOX1I:
    setParam<box1i>(handle, name, mem, type);
    break;
  case OSP_BOX2I:
    setParam<box2i>(handle, name, mem, type);
    break;
  case OSP_BOX3I:
    setParam<box3i>(handle, name, mem, type);
    break;
  case OSP_BOX4I:
    setParam<box4i>(handle, name, mem, type);
    break;
  case OSP_BOX1F:
    setParam<box1f>(handle, name, mem, type);
    break;
  case OSP_BOX2F:
    setParam<box2f>(handle, name, mem, type);
    break;
  case OSP_BOX3F:
    setParam<box3f>(handle, name, mem, type);
    break;
  case OSP_BOX4F:
    setParam<box4f>(handle, name, mem, type);
    break;
  case OSP_LINEAR2F:
    setParam<linear2f>(handle, name, mem, type);
    break;
  case OSP_LINEAR3F:
    setParam<linear3f>(handle, name, mem, type);
    break;
  case OSP_AFFINE2F:
    setParam<affine2f>(handle, name, mem, type);
    break;
  case OSP_AFFINE3F:
    setParam<affine3f>(handle, name, mem, type);
    break;
  default:
    throw std::runtime_error("Unrecognized param type!");
  }
}

void MPIOffloadDevice::removeObjectParam(OSPObject object, const char *name)
{
  const ObjectHandle handle = (const ObjectHandle &)object;

  networking::BufferWriter writer;
  writer << work::REMOVE_PARAM << handle.i64 << std::string(name);
  sendWork(writer.buffer, false);
}

void MPIOffloadDevice::commit(OSPObject _object)
{
  const ObjectHandle handle = (const ObjectHandle &)_object;

  networking::BufferWriter writer;
  writer << work::COMMIT << handle.i64;
  auto d = sharedData.find(handle.i64);
  if (d == sharedData.end()) {
    sendWork(writer.buffer, false);
  } else {
    sendDataWork(writer, d->second);
  }
}

void MPIOffloadDevice::release(OSPObject _object)
{
  const ObjectHandle handle = (const ObjectHandle &)_object;
  if (futures.find(handle.i64) != futures.end()) {
    wait((OSPFuture)_object, OSP_TASK_FINISHED);
    futures.erase(handle.i64);
  }

  networking::BufferWriter writer;
  writer << work::RELEASE << handle.i64;
  sendWork(writer.buffer, false);

  // If this object was sharing a data view with the application we need to
  // make sure the data has been transferred before letting the application
  // potentially release the buffer. We only need to flush any early data
  // sends here, since if the data was small enough to inline in the command
  // buffer we aren't actually sharing the pointer with the app anymore
  if (sharedDataViewHazard) {
    postStatusMsg(OSP_LOG_DEBUG)
        << "#osp.mpi.app: ospRelease: data reference hazard exists, "
        << " flushing pending sends";
    fabric->flushBcastSends();
    sharedDataViewHazard = false;
  }
}

void MPIOffloadDevice::retain(OSPObject _obj)
{
  const ObjectHandle handle = (const ObjectHandle &)_obj;

  networking::BufferWriter writer;
  writer << work::RETAIN << handle.i64;
  sendWork(writer.buffer, false);
}

///////////////////////////////////////////////////////////////////////////
// FrameBuffer Manipulation ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPFrameBuffer MPIOffloadDevice::frameBufferCreate(
    const vec2i &size, const OSPFrameBufferFormat format, const uint32 channels)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::CREATE_FRAMEBUFFER << handle.i64 << size << (uint32_t)format
         << channels;
  sendWork(writer.buffer, false);
  return (OSPFrameBuffer)(int64)handle;
}

OSPImageOperation MPIOffloadDevice::newImageOp(const char *type)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_IMAGE_OPERATION << handle.i64 << std::string(type);
  sendWork(writer.buffer, false);
  return (OSPImageOperation)(int64)handle;
}

const void *MPIOffloadDevice::frameBufferMap(
    OSPFrameBuffer _fb, OSPFrameBufferChannel channel)
{
  using namespace utility;
#ifdef ENABLE_PROFILING
  ProfilingPoint start;
#endif

  ObjectHandle handle = (ObjectHandle &)_fb;
  networking::BufferWriter writer;
  writer << work::MAP_FRAMEBUFFER << handle.i64 << (uint32_t)channel;
  sendWork(writer.buffer, true);

  uint64_t nbytes = 0;
  auto bytesView =
      ArrayView<uint8_t>(reinterpret_cast<uint8_t *>(&nbytes), sizeof(nbytes));

  fabric->recv(bytesView, rootWorkerRank());

  auto mapping = rkcommon::make_unique<OwnedArray<uint8_t>>();
  mapping->resize(nbytes, 0);
  fabric->recv(*mapping, rootWorkerRank());

  void *ptr = mapping->data();
  framebufferMappings[handle.i64] = std::move(mapping);
#ifdef ENABLE_PROFILING
  ProfilingPoint end;
  std::cout << "OffloadDevice::frameBufferMap took "
            << elapsedTimeMs(start, end) << "\n";
#endif

  return ptr;
}

void MPIOffloadDevice::frameBufferUnmap(const void *, OSPFrameBuffer _fb)
{
  ObjectHandle handle = (ObjectHandle &)_fb;
  auto fnd = framebufferMappings.find(handle.i64);
  if (fnd != framebufferMappings.end()) {
    framebufferMappings.erase(fnd);
  }
}

float MPIOffloadDevice::getVariance(OSPFrameBuffer _fb)
{
  using namespace utility;

  ObjectHandle handle = (ObjectHandle &)_fb;
  networking::BufferWriter writer;
  writer << work::GET_VARIANCE << handle.i64;
  sendWork(writer.buffer, true);

  float variance = 0;
  auto view = ArrayView<uint8_t>(
      reinterpret_cast<uint8_t *>(&variance), sizeof(variance));

  fabric->recv(view, rootWorkerRank());
  return variance;
}

void MPIOffloadDevice::resetAccumulation(OSPFrameBuffer _fb)
{
  ObjectHandle handle = (ObjectHandle &)_fb;
  networking::BufferWriter writer;
  writer << work::RESET_ACCUMULATION << handle.i64;
  sendWork(writer.buffer, false);
}

///////////////////////////////////////////////////////////////////////////
// Frame Rendering ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OSPRenderer MPIOffloadDevice::newRenderer(const char *type)
{
  ObjectHandle handle = allocateHandle();

  networking::BufferWriter writer;
  writer << work::NEW_RENDERER << handle.i64 << std::string(type);
  sendWork(writer.buffer, false);
  return (OSPRenderer)(int64)handle;
}

OSPFuture MPIOffloadDevice::renderFrame(OSPFrameBuffer _fb,
    OSPRenderer _renderer,
    OSPCamera _camera,
    OSPWorld _world)
{
  ObjectHandle futureHandle = allocateHandle();
  const ObjectHandle fbHandle = (ObjectHandle &)_fb;
  const ObjectHandle rendererHandle = (ObjectHandle &)_renderer;
  const ObjectHandle cameraHandle = (ObjectHandle &)_camera;
  const ObjectHandle worldHandle = (ObjectHandle &)_world;

  networking::BufferWriter writer;
  writer << work::RENDER_FRAME << fbHandle << rendererHandle << cameraHandle
         << worldHandle << futureHandle;
  sendWork(writer.buffer, true);

  futures.insert(futureHandle.i64);
  return (OSPFuture)(int64)futureHandle;
}

int MPIOffloadDevice::isReady(OSPFuture _task, OSPSyncEvent event)
{
  const ObjectHandle handle = (ObjectHandle &)_task;
  networking::BufferWriter writer;
  writer << work::FUTURE_IS_READY << handle.i64 << (uint32_t)event;
  sendWork(writer.buffer, true);

  int result = 0;
  utility::ArrayView<uint8_t> view(
      reinterpret_cast<uint8_t *>(&result), sizeof(result));
  fabric->recv(view, rootWorkerRank());
  return result;
}

void MPIOffloadDevice::wait(OSPFuture _task, OSPSyncEvent event)
{
  const ObjectHandle handle = (ObjectHandle &)_task;
  networking::BufferWriter writer;
  writer << work::FUTURE_WAIT << handle.i64 << (uint32_t)event;
  sendWork(writer.buffer, true);

  int result = 0;
  utility::ArrayView<uint8_t> view(
      reinterpret_cast<uint8_t *>(&result), sizeof(result));
  fabric->recv(view, rootWorkerRank());
}

void MPIOffloadDevice::cancel(OSPFuture _task)
{
  const ObjectHandle handle = (ObjectHandle &)_task;
  networking::BufferWriter writer;
  writer << work::FUTURE_CANCEL << handle.i64;
  sendWork(writer.buffer, false);
}

float MPIOffloadDevice::getProgress(OSPFuture _task)
{
  const ObjectHandle handle = (ObjectHandle &)_task;
  networking::BufferWriter writer;
  writer << work::FUTURE_GET_PROGRESS << handle.i64;
  sendWork(writer.buffer, true);

  float result = 0;
  utility::ArrayView<uint8_t> view(
      reinterpret_cast<uint8_t *>(&result), sizeof(result));
  fabric->recv(view, rootWorkerRank());
  return result;
}

float MPIOffloadDevice::getTaskDuration(OSPFuture _task)
{
  const ObjectHandle handle = (ObjectHandle &)_task;
  networking::BufferWriter writer;
  writer << work::FUTURE_GET_TASK_DURATION << handle.i64;
  sendWork(writer.buffer, true);

  float result = 0;
  utility::ArrayView<uint8_t> view(
      reinterpret_cast<uint8_t *>(&result), sizeof(result));
  fabric->recv(view, rootWorkerRank());
  return result;
}

OSPPickResult MPIOffloadDevice::pick(OSPFrameBuffer fb,
    OSPRenderer renderer,
    OSPCamera camera,
    OSPWorld world,
    const vec2f &screenPos)
{
  const ObjectHandle fbHandle = (ObjectHandle &)fb;
  const ObjectHandle rendererHandle = (ObjectHandle &)renderer;
  const ObjectHandle cameraHandle = (ObjectHandle &)camera;
  const ObjectHandle worldHandle = (ObjectHandle &)world;

  networking::BufferWriter writer;
  writer << work::PICK << fbHandle << rendererHandle << cameraHandle
         << worldHandle << screenPos;
  sendWork(writer.buffer, true);

  OSPPickResult result = {0};
  utility::ArrayView<uint8_t> view(
      reinterpret_cast<uint8_t *>(&result), sizeof(OSPPickResult));
  fabric->recv(view, rootWorkerRank());
  return result;
}

void MPIOffloadDevice::sendWork(
    const std::shared_ptr<utility::AbstractArray<uint8_t>> &work,
    bool submitImmediately)
{
  if (commandBufferCursor + work->size() >= commandBuffer->size()) {
    submitWork();
  }

  postStatusMsg(OSP_LOG_DEBUG)
      << "#osp.mpi.app: buffering command: "
      << work::tagName(*(reinterpret_cast<work::TAG *>(work->begin())));

  ++bufferedCommands;
  std::memcpy(commandBuffer->begin() + commandBufferCursor,
      work->begin(),
      work->size());
  commandBufferCursor += work->size();

  if (submitImmediately || bufferedCommands >= maxBufferedCommands) {
    submitWork();
  }
}

void MPIOffloadDevice::sendDataWork(rkcommon::networking::BufferWriter &writer,
    const std::shared_ptr<rkcommon::utility::AbstractArray<uint8_t>> &data)
{
  if (data->size() < maxInlineDataSize) {
    // Small data gets inline'd into the command buffer
    writer << (uint32_t)1;
    const size_t commandSize = writer.buffer->size();
    writer.buffer->resize(commandSize + data->size(), 0);
    std::memcpy(
        writer.buffer->begin() + commandSize, data->begin(), data->size());
  } else {
    // For large data we start sending it early as an early data command
    writer << (uint32_t)0;

    sharedDataViewHazard = true;

    networking::BufferWriter earlyDataCmd;
    earlyDataCmd << work::EARLY_DATA << data->size();

    networking::BufferWriter header;
    header << earlyDataCmd.buffer->size();

    fabric->sendBcast(header.buffer);
    fabric->sendBcast(earlyDataCmd.buffer);
    fabric->sendBcast(data);
  }
  sendWork(writer.buffer, false);
}

void MPIOffloadDevice::submitWork()
{
  if (commandBufferCursor == 0) {
    std::cout << "ERROR: submit on empty command buffer attempted!\n";
    return;
  }
  networking::BufferWriter header;
  header << commandBufferCursor;

  fabric->sendBcast(header.buffer);

  postStatusMsg(OSP_LOG_DEBUG)
      << "#osp.mpi.app:  submitting buffer with " << bufferedCommands
      << " commands, size: " << commandBufferCursor;

  // Make view of the valid set of commands in the buffer and submit them
  auto view = std::make_shared<utility::FixedArrayView<uint8_t>>(
      commandBuffer, 0, commandBufferCursor);
  using namespace std::chrono;
  fabric->sendBcast(view);
  bufferedCommands = 0;

  // Write new commands to a new buffer while the previous buffer is sent
  // out asynchronously
  commandBuffer = std::make_shared<rkcommon::utility::FixedArray<uint8_t>>(
      commandBufferSize);
  commandBufferCursor = 0;
}

int MPIOffloadDevice::rootWorkerRank() const
{
  if (dynamic_cast<SocketWriterFabric *>(fabric.get()))
    return 0;

  // TODO: we may also want to support MPI intercomm setups w/ comm
  // accept/connect?
  return 1;
}

ObjectHandle MPIOffloadDevice::allocateHandle() const
{
  return ObjectHandle();
}

} // namespace mpi
} // namespace ospray
