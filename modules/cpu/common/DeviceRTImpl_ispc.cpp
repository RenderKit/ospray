// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DeviceRTImpl_ispc.h"
#include "common/FeatureFlagsEnum.h"

#include "rkcommon/memory/malloc.h"

#include <cstring> // for memcpy

namespace ospray {
namespace devicert {

/////////////////////////////////////////////////////////////////////
// AsyncEvent implementation

void AsyncEventImpl::jobScheduled()
{
  std::lock_guard<std::mutex> lock(mtx);
  ready = false;
}

void AsyncEventImpl::jobStarted()
{
  std::lock_guard<std::mutex> lock(mtx);
  timer.start();
}

void AsyncEventImpl::jobFinished()
{
  {
    std::lock_guard<std::mutex> lock(mtx);
    timer.stop();
    ready = true;
  }
  cv.notify_one();
}

void AsyncEventImpl::wait() const
{
  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [this] { return ready; });
}

bool AsyncEventImpl::finished() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return ready;
}

float AsyncEventImpl::getDuration() const
{
  std::lock_guard<std::mutex> lock(mtx);
  return timer.seconds();
}

void *AsyncEventImpl::getSyclEventPtr()
{
  return nullptr;
}

/////////////////////////////////////////////////////////////////////
// Device implementation

// Base command class
struct DeviceImpl::Command
{
  Command() = default;
  Command(std::shared_ptr<AsyncEventImpl> event) : event(event) {}
  virtual ~Command() = default;

  virtual bool execute() = 0;

  friend struct DeviceImpl;

 protected:
  std::shared_ptr<AsyncEventImpl> event;
};

// Shutdown device thread command
struct DeviceImpl::CommandShutdown : public Command
{
  bool execute() override
  {
    // Just return false for loop exit
    return false;
  }
};

// MemCpy command
struct DeviceImpl::CommandMemCpy : public Command
{
  CommandMemCpy(void *dest,
      const void *src,
      std::size_t size,
      std::shared_ptr<AsyncEventImpl> event)
      : Command(event), dest(dest), src(src), size(size)
  {}
  bool execute() override
  {
    // Do memory copying
    event->jobStarted();
    std::memcpy(dest, src, size);
    event->jobFinished();

    // Continue commands processing
    return true;
  }

 private:
  void *const dest;
  const void *const src;
  const std::size_t size;
};

// Launch render kernel command
struct DeviceImpl::CommandLaunchRenderKernel : public Command
{
  CommandLaunchRenderKernel(const vec3ui &itemDims,
      RendererKernel kernel,
      ispc::Renderer *renderer,
      ispc::FrameBuffer *fb,
      ispc::Camera *camera,
      ispc::World *world,
      const uint32_t *taskIDs,
      const FeatureFlags &ff,
      std::shared_ptr<AsyncEventImpl> event)
      : Command(event),
        itemDims(itemDims),
        rendererKernel(kernel),
        renderer(renderer),
        fb(fb),
        camera(camera),
        world(world),
        taskIDs(taskIDs),
        ff(ff)
  {}
  bool execute() override
  {
    // Run kernel
    event->jobStarted();
    rendererKernel(
        nullptr, nullptr, itemDims, renderer, fb, camera, world, taskIDs, ff);
    event->jobFinished();

    // Continue commands processing
    return true;
  }

 private:
  // Domain and kernel pointer
  const vec3ui itemDims;
  RendererKernel rendererKernel;

  // Kernel parameters
  ispc::Renderer *renderer;
  ispc::FrameBuffer *fb;
  ispc::Camera *camera;
  ispc::World *world;
  const uint32_t *taskIDs;
  const FeatureFlags ff;
};

// Launch FrameOp kernel command
struct DeviceImpl::CommandLaunchFrameOpKernel : public Command
{
  CommandLaunchFrameOpKernel(const vec2ui &itemDims,
      FrameOpKernel kernel,
      const ispc::FrameBufferView *fbv,
      std::shared_ptr<AsyncEventImpl> event)
      : Command(event), itemDims(itemDims), frameOpKernel(kernel), fbv(fbv)
  {}
  bool execute() override
  {
    // Run kernel
    event->jobStarted();
    frameOpKernel(nullptr, nullptr, itemDims, fbv);
    event->jobFinished();

    // Continue commands processing
    return true;
  }

 private:
  // Domain and kernel pointer
  const vec2ui itemDims;
  FrameOpKernel frameOpKernel;

  // Kernel parameters
  const ispc::FrameBufferView *fbv;
};

// Run host task command
struct DeviceImpl::CommandRunHostTask : public Command
{
  CommandRunHostTask(
      const std::function<void()> task, std::shared_ptr<AsyncEventImpl> event)
      : Command(event), hostTask(task)
  {}
  bool execute() override
  {
    // Run task
    event->jobStarted();
    hostTask();
    event->jobFinished();

    // Continue commands processing
    return true;
  }

 private:
  const std::function<void()> hostTask;
};

DeviceImpl::DeviceImpl(bool debug)
    : Device(debug), workerThread([this]() {
        // Main worker thread loop
        CommandPtr cmd;
        do {
          // Lock the mutex
          std::unique_lock<std::mutex> lock(queueMtx);

          // Atomically release the mutex and wait. At this point, the mutex
          // is released, allowing main thread to acquire it
          queueCV.wait(lock, [this] { return !commandQueue.empty(); });

          // When notified by the main thread and the condition is true, the
          // mutex is re-acquired by the wait function before returning so we
          // can safely retrieve command from the queue
          cmd = std::move(commandQueue.front());
          commandQueue.pop();

          // Process the command
        } while (cmd->execute());
      })
{}

DeviceImpl::DeviceImpl(uint32_t, bool debug) : DeviceImpl(debug) {}

DeviceImpl::DeviceImpl(void *, void *, bool debug) : DeviceImpl(debug) {}

DeviceImpl::~DeviceImpl()
{
  // Signal the worker thread to terminate by adding shutdown command
  scheduleCommand(std::unique_ptr<Command>(new CommandShutdown()));

  // And finish worker thread
  workerThread.join();
}

void DeviceImpl::scheduleCommand(CommandPtr cmd)
{
  {
    std::lock_guard<std::mutex> lock(queueMtx);
    if (cmd->event)
      cmd->event->jobScheduled();
    commandQueue.push(std::move(cmd));
  }
  queueCV.notify_one();
}

void *DeviceImpl::deviceMalloc(std::size_t size)
{
  return rkcommon::memory::alignedMalloc(size);
}

void *DeviceImpl::sharedMalloc(std::size_t size)
{
  return rkcommon::memory::alignedMalloc(size);
}

void *DeviceImpl::hostMalloc(std::size_t size)
{
  return rkcommon::memory::alignedMalloc(size);
}

void DeviceImpl::free(void *ptr)
{
  rkcommon::memory::alignedFree(ptr);
}

Alloc DeviceImpl::getPointerType(void *) const
{
  return Alloc::Host;
}

void DeviceImpl::wait()
{
  // Get the last event if exists
  std::shared_ptr<AsyncEventImpl> lastEvent;
  {
    std::lock_guard<std::mutex> lock(queueMtx);
    if (commandQueue.empty())
      return;

    lastEvent = commandQueue.back()->event;
  }

  // And wait for its completion
  lastEvent->wait();
}

AsyncEvent DeviceImpl::createAsyncEvent()
{
  std::shared_ptr<AsyncEventImpl> eventImpl =
      std::make_shared<AsyncEventImpl>();
  return AsyncEvent(eventImpl);
}

AsyncEvent DeviceImpl::memcpy(void *dest, const void *src, std::size_t size)
{
  // Create a command with event
  auto eventImpl = std::make_shared<AsyncEventImpl>();
  CommandPtr cmd =
      std::unique_ptr<Command>(new CommandMemCpy(dest, src, size, eventImpl));

  // Put the command into the queue and signal the worker thread
  scheduleCommand(std::move(cmd));
  return AsyncEvent(eventImpl);
}

AsyncEvent DeviceImpl::launchRendererKernel(const vec3ui &itemDims,
    RendererKernel kernel,
    ispc::Renderer *renderer,
    ispc::FrameBuffer *fb,
    ispc::Camera *camera,
    ispc::World *world,
    const uint32_t *taskIDs,
    const FeatureFlags &ff)
{
  // Create a command with event
  auto eventImpl = std::make_shared<AsyncEventImpl>();
  CommandPtr cmd = std::unique_ptr<Command>(new CommandLaunchRenderKernel(
      itemDims, kernel, renderer, fb, camera, world, taskIDs, ff, eventImpl));

  // Put the command into the queue and signal the worker thread
  scheduleCommand(std::move(cmd));
  return AsyncEvent(eventImpl);
}

AsyncEvent DeviceImpl::launchFrameOpKernel(const vec2ui &itemDims,
    FrameOpKernel kernel,
    const ispc::FrameBufferView *fbv)
{
  // Create a command with event
  auto eventImpl = std::make_shared<AsyncEventImpl>();
  CommandPtr cmd = std::unique_ptr<Command>(
      new CommandLaunchFrameOpKernel(itemDims, kernel, fbv, eventImpl));

  // Put the command into the queue and signal the worker thread
  scheduleCommand(std::move(cmd));
  return AsyncEvent(eventImpl);
}

AsyncEvent DeviceImpl::launchHostTask(const std::function<void()> &task)
{
  // Create a command with event
  auto eventImpl = std::make_shared<AsyncEventImpl>();
  CommandPtr cmd =
      std::unique_ptr<Command>(new CommandRunHostTask(task, eventImpl));

  // Put the command into the queue and signal the worker thread
  scheduleCommand(std::move(cmd));
  return AsyncEvent(eventImpl);
}

void *DeviceImpl::getSyclDevicePtr()
{
  // SYCL not used
  return nullptr;
}

void *DeviceImpl::getSyclContextPtr()
{
  // SYCL not used
  return nullptr;
}

void *DeviceImpl::getSyclQueuePtr()
{
  // SYCL not used
  return nullptr;
}

} // namespace devicert
} // namespace ospray
