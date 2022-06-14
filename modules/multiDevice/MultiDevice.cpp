// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "MultiDevice.h"
#include "MultiDeviceFrameBuffer.h"
#include "MultiDeviceRenderTask.h"
#include "camera/registration.h"
#include "fb/LocalFB.h"
#include "fb/SparseFB.h"
#include "fb/registration.h"
#include "geometry/registration.h"
#include "lights/registration.h"
#include "render/LoadBalancer.h"
#include "render/RenderTask.h"
#include "render/registration.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/tasking/tasking_system_init.h"
#include "rkcommon/utility/CodeTimer.h"
#include "rkcommon/utility/getEnvVar.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "texture/registration.h"
#include "volume/transferFunction/registration.h"

namespace ospray {
namespace api {

/////////////////////////////////////////////////////////////////////////
// ManagedObject Implementation /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
void MultiDevice::commit()
{
  Device::commit();

  if (subdevices.empty()) {
    auto OSPRAY_NUM_SUBDEVICES =
        utility::getEnvVar<int>("OSPRAY_NUM_SUBDEVICES");
    int numSubdevices =
        OSPRAY_NUM_SUBDEVICES.value_or(getParam("numSubdevices", 1));

    postStatusMsg(OSP_LOG_DEBUG) << "# of subdevices =" << numSubdevices;

    std::vector<std::shared_ptr<TiledLoadBalancer>> subdeviceLoadBalancers;
    for (int i = 0; i < numSubdevices; ++i) {
      auto d = make_unique<ISPCDevice>();
      d->commit();
      subdevices.emplace_back(std::move(d));
      subdeviceLoadBalancers.push_back(subdevices.back()->loadBalancer);
    }
    loadBalancer =
        rkcommon::make_unique<MultiDeviceLoadBalancer>(subdeviceLoadBalancers);
  }

  // ISPCDevice::commit will init the tasking system but here we can reset
  // it to the global desired number of threads
  tasking::initTaskingSystem(numThreads, true);
}

/////////////////////////////////////////////////////////////////////////
// Device Implementation ////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

int MultiDevice::loadModule(const char *name)
{
  OSPError err = OSP_NO_ERROR;
  for (auto &d : subdevices) {
    auto e = d->loadModule(name);
    if (e != OSP_NO_ERROR) {
      err = (OSPError)e;
    }
  }
  return err;
}

// OSPRay Data Arrays ///////////////////////////////////////////////////

OSPData MultiDevice::newSharedData(const void *sharedData,
    OSPDataType type,
    const vec3ul &numItems,
    const vec3l &byteStride)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  // Data arrays of OSPRay objects need to populate the corresponding subdevice
  // data arrays with the objects for that subdevice
  if (type & OSP_OBJECT) {
    for (size_t i = 0; i < subdevices.size(); ++i) {
      o->objects.push_back((OSPObject) new Data(type, numItems));
    }

    // A little lazy here, but using the Data object to just give me a view
    // + the index sequence iterator to use to step over the stride
    Data *multiData = new Data(sharedData, type, numItems, byteStride);
    o->sharedDataDirtyReference = multiData;

    index_sequence_3D seq(numItems);
    for (auto idx : seq) {
      MultiDeviceObject *mobj = *(MultiDeviceObject **)multiData->data(idx);
      retain((OSPObject)mobj);

      // Copy the subdevice object handles to the data arrays for each subdevice
      for (size_t i = 0; i < subdevices.size(); ++i) {
        Data *subdeviceData = (Data *)o->objects[i];
        std::memcpy(
            subdeviceData->data(idx), &mobj->objects[i], sizeof(OSPObject));
      }
    }
  } else {
    for (auto &d : subdevices) {
      o->objects.push_back(
          d->newSharedData(sharedData, type, numItems, byteStride));
    }
  }
  return (OSPData)o;
}

OSPData MultiDevice::newData(OSPDataType type, const vec3ul &numItems)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newData(type, numItems));
  }
  return (OSPData)o;
}

void MultiDevice::copyData(
    const OSPData source, OSPData destination, const vec3ul &destinationIndex)
{
  const MultiDeviceObject *srcs = (const MultiDeviceObject *)source;
  MultiDeviceObject *dsts = (MultiDeviceObject *)destination;
  for (size_t i = 0; i < subdevices.size(); ++i) {
    subdevices[i]->copyData(
        (OSPData)srcs->objects[i], (OSPData)dsts->objects[i], destinationIndex);
  }
}

// Renderable Objects ///////////////////////////////////////////////////

OSPLight MultiDevice::newLight(const char *type)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newLight(type));
  }
  return (OSPLight)o;
}

OSPCamera MultiDevice::newCamera(const char *type)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newCamera(type));
  }
  return (OSPCamera)o;
}

OSPGeometry MultiDevice::newGeometry(const char *type)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newGeometry(type));
  }
  return (OSPGeometry)o;
}

OSPVolume MultiDevice::newVolume(const char *type)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newVolume(type));
  }
  return (OSPVolume)o;
}

OSPGeometricModel MultiDevice::newGeometricModel(OSPGeometry geom)
{
  MultiDeviceObject *g = (MultiDeviceObject *)geom;
  MultiDeviceObject *o = new MultiDeviceObject;
  for (size_t i = 0; i < subdevices.size(); ++i) {
    OSPGeometry subdeviceObj =
        g ? (OSPGeometry)g->objects[i] : (OSPGeometry) nullptr;
    o->objects.push_back(subdevices[i]->newGeometricModel(subdeviceObj));
  }
  return (OSPGeometricModel)o;
}
OSPVolumetricModel MultiDevice::newVolumetricModel(OSPVolume volume)
{
  MultiDeviceObject *v = (MultiDeviceObject *)volume;
  MultiDeviceObject *o = new MultiDeviceObject;
  for (size_t i = 0; i < subdevices.size(); ++i) {
    OSPVolume subdeviceObj = v ? (OSPVolume)v->objects[i] : (OSPVolume) nullptr;
    o->objects.push_back(subdevices[i]->newVolumetricModel(subdeviceObj));
  }
  return (OSPVolumetricModel)o;
}

// Model Meta-Data //////////////////////////////////////////////////////

OSPMaterial MultiDevice::newMaterial(const char *, const char *material_type)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newMaterial(nullptr, material_type));
  }
  return (OSPMaterial)o;
}

OSPTransferFunction MultiDevice::newTransferFunction(const char *type)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newTransferFunction(type));
  }
  return (OSPTransferFunction)o;
}

OSPTexture MultiDevice::newTexture(const char *type)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newTexture(type));
  }
  return (OSPTexture)o;
}

// Instancing ///////////////////////////////////////////////////////////

OSPGroup MultiDevice::newGroup()
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newGroup());
  }
  return (OSPGroup)o;
}

OSPInstance MultiDevice::newInstance(OSPGroup group)
{
  MultiDeviceObject *g = (MultiDeviceObject *)group;
  MultiDeviceObject *o = new MultiDeviceObject;
  for (size_t i = 0; i < subdevices.size(); ++i) {
    OSPGroup subdeviceObj = g ? (OSPGroup)g->objects[i] : (OSPGroup) nullptr;
    o->objects.push_back(subdevices[i]->newInstance(subdeviceObj));
  }
  return (OSPInstance)o;
}

// Top-level Worlds /////////////////////////////////////////////////////

OSPWorld MultiDevice::newWorld()
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newWorld());
  }
  return (OSPWorld)o;
}

box3f MultiDevice::getBounds(OSPObject obj)
{
  MultiDeviceObject *o = (MultiDeviceObject *)obj;
  // Everything is replicated across the subdevices, so we
  // can just query the bounds from one of them
  return subdevices[0]->getBounds(o->objects[0]);
}

// Object + Parameter Lifetime Management ///////////////////////////////

void MultiDevice::setObjectParam(
    OSPObject object, const char *name, OSPDataType type, const void *mem)
{
  // If the type is an OSPObject, we need to find the per-subdevice objects
  // and set them appropriately
  MultiDeviceObject *o = (MultiDeviceObject *)object;
  if (type & OSP_OBJECT) {
    MultiDeviceObject *p = *(MultiDeviceObject **)mem;
    for (size_t i = 0; i < subdevices.size(); ++i) {
      subdevices[i]->setObjectParam(o->objects[i], name, type, &p->objects[i]);
    }
  } else {
    for (size_t i = 0; i < subdevices.size(); ++i) {
      subdevices[i]->setObjectParam(o->objects[i], name, type, mem);
    }
  }
}

void MultiDevice::removeObjectParam(OSPObject object, const char *name)
{
  MultiDeviceObject *o = (MultiDeviceObject *)object;
  for (size_t i = 0; i < subdevices.size(); ++i) {
    subdevices[i]->removeObjectParam(o->objects[i], name);
  }
}

void MultiDevice::commit(OSPObject object)
{
  MultiDeviceObject *o = (MultiDeviceObject *)object;

  // Applications can change their shared buffer contents directly,
  // so shared arrays of objects have to do more to ensure that the
  // contents are up to date. Specifically the handles have to be
  // translated down to the subdevice specific handles correctly.
  if (o->sharedDataDirtyReference) {
    Data *multiData = o->sharedDataDirtyReference;
    const vec3ul &numItems = multiData->numItems;
    index_sequence_3D seq(numItems);
    for (auto idx : seq) {
      MultiDeviceObject *mobj = *(MultiDeviceObject **)multiData->data(idx);
      retain((OSPObject)mobj);

      // Copy the subdevice object handles to the data arrays for each subdevice
      for (size_t i = 0; i < subdevices.size(); ++i) {
        Data *subdeviceData = (Data *)o->objects[i];
        std::memcpy(
            subdeviceData->data(idx), &mobj->objects[i], sizeof(OSPObject));
      }
    }
  }

  for (size_t i = 0; i < subdevices.size(); ++i) {
    subdevices[i]->commit(o->objects[i]);
  }
}

void MultiDevice::release(OSPObject object)
{
  memory::RefCount *o = (memory::RefCount *)object;
  MultiDeviceObject *mo = dynamic_cast<MultiDeviceObject *>(o);
  if (mo) {
    if (mo->sharedDataDirtyReference) {
      Data *multiData = mo->sharedDataDirtyReference;
      const vec3ul &numItems = multiData->numItems;
      index_sequence_3D seq(numItems);
      for (auto idx : seq) {
        MultiDeviceObject *mobj = *(MultiDeviceObject **)multiData->data(idx);
        mobj->refDec();
      }
    }
    for (size_t i = 0; i < mo->objects.size(); ++i) {
      subdevices[i]->release(mo->objects[i]);
    }
  }
  o->refDec();
}

void MultiDevice::retain(OSPObject object)
{
  memory::RefCount *o = (memory::RefCount *)object;
  MultiDeviceObject *mo = dynamic_cast<MultiDeviceObject *>(o);
  if (mo) {
    for (size_t i = 0; i < mo->objects.size(); ++i) {
      subdevices[i]->retain(mo->objects[i]);
    }
  }
  o->refInc();
}

// FrameBuffer Manipulation /////////////////////////////////////////////

OSPFrameBuffer MultiDevice::frameBufferCreate(
    const vec2i &size, const OSPFrameBufferFormat mode, const uint32 channels)
{
  MultiDeviceFrameBuffer *o = new MultiDeviceFrameBuffer();
  o->rowmajorFb = new LocalFrameBuffer(size, mode, channels);
  // Need one refDec here for the local scope ref (see issue about Ref<>)
  o->rowmajorFb->refDec();

  const vec2i totalTiles = divRoundUp(size, vec2i(TILE_SIZE));
  for (size_t i = 0; i < subdevices.size(); ++i) {
    // Assign tiles round-robin among the subdevices
    std::vector<uint32_t> tileIDs;
    for (uint32_t tid = 0; tid < totalTiles.long_product(); ++tid) {
      if (tid % subdevices.size() == i) {
        tileIDs.push_back(tid);
      }
    }

    FrameBuffer *fbi = new SparseFrameBuffer(size, mode, channels, tileIDs);
    o->objects.push_back((OSPFrameBuffer)fbi);
  }
  return (OSPFrameBuffer)o;
}

OSPImageOperation MultiDevice::newImageOp(const char *type)
{
  // Same note for image ops as for framebuffers in terms of how they are
  // treated as shared. Eventually we would have per hardware device ones though
  // for cpu/gpus
  auto *op = ImageOp::createInstance(type);
  MultiDeviceObject *o = new MultiDeviceObject();
  for (size_t i = 0; i < subdevices.size(); ++i) {
    o->objects.push_back((OSPImageOperation)op);
  }
  return (OSPImageOperation)o;
}

const void *MultiDevice::frameBufferMap(
    OSPFrameBuffer _fb, const OSPFrameBufferChannel channel)
{
  MultiDeviceFrameBuffer *o = (MultiDeviceFrameBuffer *)_fb;
  return o->rowmajorFb->mapBuffer(channel);
}

void MultiDevice::frameBufferUnmap(const void *mapped, OSPFrameBuffer _fb)
{
  MultiDeviceFrameBuffer *o = (MultiDeviceFrameBuffer *)_fb;
  o->rowmajorFb->unmap(mapped);
}

float MultiDevice::getVariance(OSPFrameBuffer _fb)
{
  MultiDeviceFrameBuffer *o = (MultiDeviceFrameBuffer *)_fb;
  return o->rowmajorFb->getVariance();
}

void MultiDevice::resetAccumulation(OSPFrameBuffer _fb)
{
  MultiDeviceFrameBuffer *o = (MultiDeviceFrameBuffer *)_fb;
  o->rowmajorFb->clear();
  for (size_t i = 0; i < subdevices.size(); ++i) {
    SparseFrameBuffer *fbi = (SparseFrameBuffer *)o->objects[i];
    fbi->clear();
  }
}

// Frame Rendering //////////////////////////////////////////////////////

OSPRenderer MultiDevice::newRenderer(const char *type)
{
  MultiDeviceObject *o = new MultiDeviceObject;
  for (auto &d : subdevices) {
    o->objects.push_back(d->newRenderer(type));
  }
  return (OSPRenderer)o;
}

OSPFuture MultiDevice::renderFrame(OSPFrameBuffer _framebuffer,
    OSPRenderer _renderer,
    OSPCamera _camera,
    OSPWorld _world)
{
  MultiDeviceFrameBuffer *multiFb = (MultiDeviceFrameBuffer *)_framebuffer;
  multiFb->rowmajorFb->setCompletedEvent(OSP_NONE_FINISHED);
  for (size_t i = 0; i < multiFb->objects.size(); ++i) {
    SparseFrameBuffer *fbi = (SparseFrameBuffer *)multiFb->objects[i];
    fbi->setCompletedEvent(OSP_NONE_FINISHED);
  }

  MultiDeviceObject *multiRenderer = (MultiDeviceObject *)_renderer;
  MultiDeviceObject *multiCamera = (MultiDeviceObject *)_camera;
  MultiDeviceObject *multiWorld = (MultiDeviceObject *)_world;

  retain((OSPObject)multiFb);
  retain((OSPObject)multiRenderer);
  retain((OSPObject)multiCamera);
  retain((OSPObject)multiWorld);

  auto *f = new MultiDeviceRenderTask(multiFb, [=]() {
    utility::CodeTimer timer;
    timer.start();
    loadBalancer->renderFrame(multiFb, multiRenderer, multiCamera, multiWorld);
    timer.stop();

    release((OSPObject)multiFb);
    release((OSPObject)multiRenderer);
    release((OSPObject)multiCamera);
    release((OSPObject)multiWorld);

    return timer.seconds();
  });

  return (OSPFuture)f;
}

int MultiDevice::isReady(OSPFuture _task, OSPSyncEvent event)
{
  auto *task = (Future *)_task;
  return task->isFinished(event);
}

void MultiDevice::wait(OSPFuture _task, OSPSyncEvent event)
{
  auto *task = (Future *)_task;
  task->wait(event);
}

void MultiDevice::cancel(OSPFuture _task)
{
  auto *task = (Future *)_task;
  return task->cancel();
}

float MultiDevice::getProgress(OSPFuture _task)
{
  auto *task = (Future *)_task;
  return task->getProgress();
}

float MultiDevice::getTaskDuration(OSPFuture _task)
{
  auto *task = (Future *)_task;
  return task->getTaskDuration();
}

OSPPickResult MultiDevice::pick(OSPFrameBuffer _fb,
    OSPRenderer _renderer,
    OSPCamera _camera,
    OSPWorld _world,
    const vec2f &screenPos)
{
  MultiDeviceObject *multiFb = (MultiDeviceObject *)_fb;
  FrameBuffer *fb = (FrameBuffer *)multiFb->objects[0];

  MultiDeviceObject *multiRenderer = (MultiDeviceObject *)_renderer;
  MultiDeviceObject *multiCamera = (MultiDeviceObject *)_camera;
  MultiDeviceObject *multiWorld = (MultiDeviceObject *)_world;

  // Data in the multidevice is all replicated, so we just run the pick on the
  // first subdevice
  return subdevices[0]->pick((OSPFrameBuffer)fb,
      (OSPRenderer)multiRenderer->objects[0],
      (OSPCamera)multiCamera->objects[0],
      (OSPWorld)multiWorld->objects[0],
      screenPos);
}

} // namespace api
} // namespace ospray
