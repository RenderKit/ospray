// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <vector>

#include "../fb/DistributedFrameBuffer.h"
#include "MPICommon.h"
#include "OSPWork.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/ObjectHandle.h"
#include "common/World.h"
#include "geometry/GeometricModel.h"
#include "ospray/MPIDistributedDevice.h"
#include "render/RenderTask.h"
#include "rkcommon/array3D/for_each.h"
#include "rkcommon/utility/ArrayView.h"
#include "rkcommon/utility/OwnedArray.h"
#include "texture/Texture.h"
#include "volume/VolumetricModel.h"

namespace ospray {
namespace mpi {
namespace work {

FrameBufferInfo::FrameBufferInfo(
    const vec2i &size, OSPFrameBufferFormat format, uint32_t channels)
    : size(size), format(format), channels(channels)
{}

size_t FrameBufferInfo::pixelSize(uint32_t channel) const
{
  switch (channel) {
  case OSP_FB_COLOR:
    switch (format) {
    case OSP_FB_RGBA8:
    case OSP_FB_SRGBA:
      return sizeof(uint32_t);
    case OSP_FB_RGBA32F:
      return sizeof(vec4f);
    default:
      return 0;
    }
  case OSP_FB_DEPTH:
    return channels & OSP_FB_DEPTH ? sizeof(float) : 0;
  case OSP_FB_NORMAL:
    return channels & OSP_FB_NORMAL ? sizeof(vec3f) : 0;
  case OSP_FB_ALBEDO:
    return channels & OSP_FB_ALBEDO ? sizeof(vec3f) : 0;
  default:
    return 0;
  }
}

size_t FrameBufferInfo::getNumPixels() const
{
  return size.x * size.y;
}

Data *OSPState::getSharedDataHandle(int64_t handle) const
{
  auto fnd = appSharedData.find(handle);
  if (fnd != appSharedData.end()) {
    return fnd->second;
  }
  return nullptr;
}

void newRenderer(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewRenderer(type.c_str());
}

void newWorld(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  state.objects[handle] = ospNewWorld();
}

void newGeometry(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewGeometry(type.c_str());
}

void newGeometricModel(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  int64_t geomHandle = 0;
  cmdBuf >> handle >> geomHandle;
  state.objects[handle] =
      ospNewGeometricModel(state.getObject<OSPGeometry>(geomHandle));
}

void newVolume(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewVolume(type.c_str());
}

void newVolumetricModel(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  int64_t volHandle = 0;
  cmdBuf >> handle >> volHandle;
  state.objects[handle] =
      ospNewVolumetricModel(state.getObject<OSPVolume>(volHandle));
}

void newCamera(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewCamera(type.c_str());
}

void newTransferFunction(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewTransferFunction(type.c_str());
}

void newImageOperation(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewImageOperation(type.c_str());
}

void newMaterial(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type, rendererType;
  cmdBuf >> handle >> rendererType >> type;
  state.objects[handle] = ospNewMaterial(rendererType.c_str(), type.c_str());
}

void newLight(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewLight(type.c_str());
}

void dataTransfer(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  using namespace utility;

  OSPDataType type;
  vec3ul numItems = 0;
  cmdBuf >> type >> numItems;

  Data *data = new Data(type, numItems);

  const uint64_t nbytes = data->size() * sizeOf(type);
  auto view = std::make_shared<ArrayView<uint8_t>>(
      reinterpret_cast<uint8_t *>(data->data()), nbytes);
  fabric.recvBcast(*view);

  state.dataTransfers.push(data);
}

Data *retrieveData(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric,
    const OSPDataType type,
    const vec3ul numItems,
    Data *outputData)
{
  using namespace utility;
  uint32_t dataInline = 0;
  cmdBuf >> dataInline;
  const uint64_t nbytes = numItems.x * numItems.y * numItems.z * sizeOf(type);
  if (dataInline) {
    // If the data is inline we copy it out of the command buffer into
    // a fixed array, since the command buffer will be destroyed after
    // processing it
    if (!outputData) {
      outputData = new Data(type, numItems);
    }
    cmdBuf.read(outputData->data(), nbytes);
  } else {
    // All large data is sent before the command buffer using it, and will be
    // in the state's data transfers list in order by the command referencing it
    auto data = state.dataTransfers.front();
    state.dataTransfers.pop();

    if (outputData) {
      // All data on the workers is compact, with the compaction done by the
      // app rank before sending
      std::memcpy(outputData->data(), data->data(), nbytes);
    } else {
      outputData = data;
    }
  }

  // If the data type is managed we need to convert the handles back into
  // OSPObjects and increment the refcount because we're populating the data
  // object manually
  if (mpicommon::isManagedObject(type)) {
    for (size_t i = 0; i < array3D::longProduct(numItems); ++i) {
      char *addr = outputData->data() + i * sizeOf(type);
      int64_t *h = reinterpret_cast<int64_t *>(addr);
      OSPObject *obj = reinterpret_cast<OSPObject *>(addr);
      *obj = state.objects[*h];

      ManagedObject *m = lookupObject<ManagedObject>(*obj);
      m->refInc();
    }
  }

  return outputData;
}

void newSharedData(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  using namespace utility;

  int64_t handle = 0;
  OSPDataType format;
  vec3ul numItems = 0;
  cmdBuf >> handle >> format >> numItems;

  auto data = retrieveData(state, cmdBuf, fabric, format, numItems, nullptr);

  state.objects[handle] = (OSPData)data;
  state.appSharedData[handle] = data;
}

void newData(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  OSPDataType format;
  vec3ul numItems = 0;
  cmdBuf >> handle >> format >> numItems;

  state.objects[handle] =
      ospNewData(format, numItems.x, numItems.y, numItems.z);
}

void copyData(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t sourceHandle = 0;
  int64_t destinationHandle = 0;
  vec3ul destinationIndex = 0;
  cmdBuf >> sourceHandle >> destinationHandle >> destinationIndex;

  ospCopyData(state.getObject<OSPData>(sourceHandle),
      state.getObject<OSPData>(destinationHandle),
      destinationIndex.x,
      destinationIndex.y,
      destinationIndex.z);
}

void newTexture(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewTexture(type.c_str());
}

void newGroup(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  state.objects[handle] = ospNewGroup();
}

void newInstance(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  int64_t groupHandle = 0;
  cmdBuf >> handle >> groupHandle;
  state.objects[handle] =
      ospNewInstance(state.getObject<OSPGroup>(groupHandle));
}

void commit(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  cmdBuf >> handle;

  // If it's a data being committed, we need to retrieve the updated data
  Data *d = state.getSharedDataHandle(handle);
  if (d) {
    retrieveData(state, cmdBuf, fabric, d->type, d->numItems, d);
  }

  ospCommit(state.objects[handle]);
}

void release(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  ospRelease(state.objects[handle]);
  // Note: we keep the handle in the state.objects list as it may be referenced
  // by other objects in the scene as a parameter or data.

  // Check if we can release a referenced framebuffer info
  {
    auto fnd = state.framebuffers.find(handle);
    if (fnd != state.framebuffers.end()) {
      OSPObject obj = state.objects[handle];
      ManagedObject *m = lookupDistributedObject<ManagedObject>(obj);
      // Framebuffers are given an extra ref count by the worker so that
      // we can track the lifetime of their framebuffer info. Use count == 1
      // means only the worker rank has a reference to the object
      if (m->useCount() == 1) {
        ospRelease(state.objects[handle]);
        state.framebuffers.erase(fnd);
      }
    }
  }

  if (state.getSharedDataHandle(handle)) {
    state.appSharedData.erase(handle);
  }
}

void retain(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  ospRetain(state.objects[handle]);
}

void loadModule(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  std::string module;
  cmdBuf >> module;
  ospLoadModule(module.c_str());
}

void createFramebuffer(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  vec2i size(0, 0);
  uint32_t format;
  uint32_t channels = 0;
  cmdBuf >> handle >> size >> format >> channels;
  state.objects[handle] =
      ospNewFrameBuffer(size.x, size.y, (OSPFrameBufferFormat)format, channels);
  state.framebuffers[handle] =
      FrameBufferInfo(size, (OSPFrameBufferFormat)format, channels);

  // Offload device keeps +1 ref for tracking the lifetime of the framebuffer
  ospRetain(state.objects[handle]);
}

void mapFramebuffer(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  // Map the channel and send the image back over the fabric
  int64_t handle = 0;
  uint32_t channel = 0;
  cmdBuf >> handle >> channel;

  if (mpicommon::worker.rank == 0) {
    using namespace utility;

    const FrameBufferInfo &fbInfo = state.framebuffers[handle];
    uint64_t nbytes = fbInfo.pixelSize(channel) * fbInfo.getNumPixels();

    auto bytesView = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&nbytes), sizeof(nbytes));
    fabric.send(bytesView, 0);

    if (nbytes != 0) {
      OSPFrameBuffer fb = state.getObject<OSPFrameBuffer>(handle);
      void *map = const_cast<void *>(
          ospMapFrameBuffer(fb, (OSPFrameBufferChannel)channel));

      auto fbView = std::make_shared<OwnedArray<uint8_t>>(
          reinterpret_cast<uint8_t *>(map), nbytes);

      fabric.send(fbView, 0);
      ospUnmapFrameBuffer(map, fb);
    }
  }
}

void getVariance(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  // Map the channel and send the image back over the fabric
  int64_t handle = 0;
  cmdBuf >> handle;

  if (mpicommon::worker.rank == 0) {
    using namespace utility;

    float variance = ospGetVariance(state.getObject<OSPFrameBuffer>(handle));

    auto bytesView = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&variance), sizeof(variance));
    fabric.send(bytesView, 0);
  }
}

void resetAccumulation(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  ospResetAccumulation(state.getObject<OSPFrameBuffer>(handle));
}

void renderFrame(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t futureHandle = 0;
  int64_t fbHandle = 0;
  int64_t rendererHandle = 0;
  int64_t cameraHandle = 0;
  int64_t worldHandle = 0;
  cmdBuf >> fbHandle >> rendererHandle >> cameraHandle >> worldHandle
      >> futureHandle;
  state.objects[futureHandle] =
      ospRenderFrame(state.getObject<OSPFrameBuffer>(fbHandle),
          state.getObject<OSPRenderer>(rendererHandle),
          state.getObject<OSPCamera>(cameraHandle),
          state.getObject<OSPWorld>(worldHandle));
}

template <typename T>
void setParam(networking::BufferReader &cmdBuf,
    OSPObject obj,
    const std::string &param,
    OSPDataType type)
{
  T val;
  cmdBuf >> val;
  ospSetParam(obj, param.c_str(), type, &val);
}

void setParam(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string param;
  OSPDataType type;
  cmdBuf >> handle >> param >> type;

  // OSP_OBJECT use the same style of setting param since it's just a handle
  if (mpicommon::isManagedObject(type)) {
    int64_t val = 0;
    cmdBuf >> val;
    ospSetParam(
        state.objects[handle], param.c_str(), type, &state.objects[val]);
  } else {
    switch (type) {
    case OSP_STRING: {
      std::string val;
      cmdBuf >> val;
      ospSetParam(state.objects[handle], param.c_str(), type, val.c_str());
      break;
    }
    case OSP_BOOL:
      setParam<bool>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_CHAR:
    case OSP_BYTE:
      setParam<char>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC2UC:
      setParam<vec2uc>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC3UC:
      setParam<vec3uc>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC4UC:
      setParam<vec4uc>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_SHORT:
      setParam<short>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_USHORT:
      setParam<unsigned short>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_INT:
      setParam<int>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC2I:
      setParam<vec2i>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC3I:
      setParam<vec3i>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC4I:
      setParam<vec4i>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_UINT:
      setParam<unsigned int>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC2UI:
      setParam<vec2ui>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC3UI:
      setParam<vec3ui>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC4UI:
      setParam<vec4ui>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_LONG:
      setParam<long>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC2L:
      setParam<vec2l>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC3L:
      setParam<vec3l>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC4L:
      setParam<vec4l>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_ULONG:
      setParam<unsigned long>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC2UL:
      setParam<vec2ul>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC3UL:
      setParam<vec3ul>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC4UL:
      setParam<vec4ul>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_FLOAT:
      setParam<float>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC2F:
      setParam<vec2f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC3F:
      setParam<vec3f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_VEC4F:
      setParam<vec4f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_DOUBLE:
      setParam<double>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_BOX1I:
      setParam<box1i>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_BOX2I:
      setParam<box2i>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_BOX3I:
      setParam<box3i>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_BOX4I:
      setParam<box4i>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_BOX1F:
      setParam<box1f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_BOX2F:
      setParam<box2f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_BOX3F:
      setParam<box3f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_BOX4F:
      setParam<box4f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_LINEAR2F:
      setParam<linear2f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_LINEAR3F:
      setParam<linear3f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_AFFINE2F:
      setParam<affine2f>(cmdBuf, state.objects[handle], param, type);
      break;
    case OSP_AFFINE3F:
      setParam<affine3f>(cmdBuf, state.objects[handle], param, type);
      break;
    default:
      throw std::runtime_error("Unrecognized param type!");
    }
  }
}

void removeParam(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string param;
  cmdBuf >> handle >> param;
  ospRemoveParam(state.objects[handle], param.c_str());
}

void setLoadBalancer(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  NOT_IMPLEMENTED;
  // TODO: We only have one load balancer now
}

void pick(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t fbHandle = 0;
  int64_t rendererHandle = 0;
  int64_t cameraHandle = 0;
  int64_t worldHandle = 0;
  vec2f screenPos;
  cmdBuf >> fbHandle >> rendererHandle >> cameraHandle >> worldHandle
      >> screenPos;

  OSPPickResult res = {0};
  ospPick(&res,
      state.getObject<OSPFrameBuffer>(fbHandle),
      state.getObject<OSPRenderer>(rendererHandle),
      state.getObject<OSPCamera>(cameraHandle),
      state.getObject<OSPWorld>(worldHandle),
      screenPos.x,
      screenPos.y);

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&res), sizeof(OSPPickResult));
    fabric.send(view, 0);
  }
}

void getBounds(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  cmdBuf >> handle;

  OSPBounds res = ospGetBounds(state.objects[handle]);

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&res), sizeof(OSPBounds));
    fabric.send(view, 0);
  }
}

void futureIsReady(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  uint32_t event = 0;
  cmdBuf >> handle >> event;
  int ready =
      ospIsReady(state.getObject<OSPFuture>(handle), (OSPSyncEvent)event);

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&ready), sizeof(ready));
    fabric.send(view, 0);
  }
}

void futureWait(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  uint32_t event = 0;
  cmdBuf >> handle >> event;
  ospWait(state.getObject<OSPFuture>(handle), (OSPSyncEvent)event);

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&event), sizeof(event));
    fabric.send(view, 0);
  }
}

void futureCancel(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  ospCancel(state.getObject<OSPFuture>(handle));
}

void futureGetProgress(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  float progress = ospGetProgress(state.getObject<OSPFuture>(handle));

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&progress), sizeof(progress));
    fabric.send(view, 0);
  }
}

void futureGetTaskDuration(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  float progress = ospGetTaskDuration(state.getObject<OSPFuture>(handle));

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&progress), sizeof(progress));
    fabric.send(view, 0);
  }
}

void dispatchWork(TAG t,
    OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  switch (t) {
  case NEW_RENDERER:
    newRenderer(state, cmdBuf, fabric);
    break;
  case NEW_WORLD:
    newWorld(state, cmdBuf, fabric);
    break;
  case NEW_GEOMETRY:
    newGeometry(state, cmdBuf, fabric);
    break;
  case NEW_GEOMETRIC_MODEL:
    newGeometricModel(state, cmdBuf, fabric);
    break;
  case NEW_VOLUME:
    newVolume(state, cmdBuf, fabric);
    break;
  case NEW_VOLUMETRIC_MODEL:
    newVolumetricModel(state, cmdBuf, fabric);
    break;
  case NEW_CAMERA:
    newCamera(state, cmdBuf, fabric);
    break;
  case NEW_TRANSFER_FUNCTION:
    newTransferFunction(state, cmdBuf, fabric);
    break;
  case NEW_IMAGE_OPERATION:
    newImageOperation(state, cmdBuf, fabric);
    break;
  case NEW_MATERIAL:
    newMaterial(state, cmdBuf, fabric);
    break;
  case NEW_LIGHT:
    newLight(state, cmdBuf, fabric);
    break;
  case DATA_TRANSFER:
    dataTransfer(state, cmdBuf, fabric);
    break;
  case NEW_SHARED_DATA:
    newSharedData(state, cmdBuf, fabric);
    break;
  case NEW_DATA:
    newData(state, cmdBuf, fabric);
    break;
  case COPY_DATA:
    copyData(state, cmdBuf, fabric);
    break;
  case NEW_TEXTURE:
    newTexture(state, cmdBuf, fabric);
    break;
  case NEW_GROUP:
    newGroup(state, cmdBuf, fabric);
    break;
  case NEW_INSTANCE:
    newInstance(state, cmdBuf, fabric);
    break;
  case COMMIT:
    commit(state, cmdBuf, fabric);
    break;
  case RELEASE:
    release(state, cmdBuf, fabric);
    break;
  case RETAIN:
    retain(state, cmdBuf, fabric);
    break;
  case LOAD_MODULE:
    loadModule(state, cmdBuf, fabric);
    break;
  case CREATE_FRAMEBUFFER:
    createFramebuffer(state, cmdBuf, fabric);
    break;
  case MAP_FRAMEBUFFER:
    mapFramebuffer(state, cmdBuf, fabric);
    break;
  case GET_VARIANCE:
    getVariance(state, cmdBuf, fabric);
    break;
  case RESET_ACCUMULATION:
    resetAccumulation(state, cmdBuf, fabric);
    break;
  case RENDER_FRAME:
    renderFrame(state, cmdBuf, fabric);
    break;
  case SET_PARAM:
    setParam(state, cmdBuf, fabric);
    break;
  case REMOVE_PARAM:
    removeParam(state, cmdBuf, fabric);
    break;
  case SET_LOAD_BALANCER:
    setLoadBalancer(state, cmdBuf, fabric);
    break;
  case PICK:
    pick(state, cmdBuf, fabric);
    break;
  case GET_BOUNDS:
    getBounds(state, cmdBuf, fabric);
    break;
  case FUTURE_IS_READY:
    futureIsReady(state, cmdBuf, fabric);
    break;
  case FUTURE_WAIT:
    futureWait(state, cmdBuf, fabric);
    break;
  case FUTURE_CANCEL:
    futureCancel(state, cmdBuf, fabric);
    break;
  case FUTURE_GET_PROGRESS:
    futureGetProgress(state, cmdBuf, fabric);
    break;
  case FUTURE_GET_TASK_DURATION:
    futureGetTaskDuration(state, cmdBuf, fabric);
    break;
  case NONE:
  default:
    throw std::runtime_error("Invalid work tag!");
    break;
  }
}

const char *tagName(work::TAG t)
{
  switch (t) {
  case NEW_RENDERER:
    return "NEW_RENDERER";
  case NEW_WORLD:
    return "NEW_WORLD";
  case NEW_GEOMETRY:
    return "NEW_GEOMETRY";
  case NEW_GEOMETRIC_MODEL:
    return "NEW_GEOMETRIC_MODEL";
  case NEW_VOLUME:
    return "NEW_VOLUME";
  case NEW_VOLUMETRIC_MODEL:
    return "NEW_VOLUMETRIC_MODEL";
  case NEW_CAMERA:
    return "NEW_CAMERA";
  case NEW_TRANSFER_FUNCTION:
    return "NEW_TRANSFER_FUNCTION";
  case NEW_IMAGE_OPERATION:
    return "NEW_IMAGE_OPERATION";
  case NEW_MATERIAL:
    return "NEW_MATERIAL";
  case NEW_LIGHT:
    return "NEW_LIGHT";
  case DATA_TRANSFER:
    return "DATA_TRANSFER";
  case NEW_SHARED_DATA:
    return "NEW_SHARED_DATA";
  case NEW_DATA:
    return "NEW_DATA";
  case COPY_DATA:
    return "COPY_DATA";
  case NEW_TEXTURE:
    return "NEW_TEXTURE";
  case NEW_GROUP:
    return "NEW_GROUP";
  case NEW_INSTANCE:
    return "NEW_INSTANCE";
  case COMMIT:
    return "COMMIT";
  case RELEASE:
    return "RELEASE";
  case RETAIN:
    return "RETAIN";
  case LOAD_MODULE:
    return "LOAD_MODULE";
  case CREATE_FRAMEBUFFER:
    return "CREATE_FRAMEBUFFER";
  case MAP_FRAMEBUFFER:
    return "MAP_FRAMEBUFFER";
  case GET_VARIANCE:
    return "GET_VARIANCE";
  case RESET_ACCUMULATION:
    return "RESET_ACCUMULATION";
  case RENDER_FRAME:
    return "RENDER_FRAME";
  case SET_PARAM:
    return "SET_PARAM";
  case REMOVE_PARAM:
    return "REMOVE_PARAM";
  case SET_LOAD_BALANCER:
    return "SET_LOAD_BALANCER";
  case PICK:
    return "PICK";
  case GET_BOUNDS:
    return "GET_BOUNDS";
  case FUTURE_IS_READY:
    return "FUTURE_IS_READY";
  case FUTURE_WAIT:
    return "FUTURE_WAIT";
  case FUTURE_CANCEL:
    return "FUTURE_CANCEL";
  case FUTURE_GET_PROGRESS:
    return "FUTURE_GET_PROGRESS";
  case FINALIZE:
    return "FINALIZE";
  case NONE:
  default:
    return "NONE/UNKNOWN/INVALID";
  }
}

} // namespace work
} // namespace mpi
} // namespace ospray
