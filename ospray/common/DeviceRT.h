// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "OSPCommon.h" // OSPRAY_CORE_INTERFACE

#include <cstddef> // size_t
#include <functional>
#include <memory> // unique_ptr, shared_ptr
#include <vector>

// Forward declaractions for types used during kernel launching
namespace ispc {
struct Renderer;
struct FrameBuffer;
struct Camera;
struct World;
struct FrameBufferView;
} // namespace ispc

namespace ospray {

struct FeatureFlags;

namespace devicert {

enum class Alloc
{
  Host = 0,
  Device = 1,
  Shared = 2,
  Unknown = 3
};

// Renderer kernel type
using RendererKernel = void (*)(void *,
    void *,
    const vec3ui &,
    void *,
    void *,
    void *,
    void *,
    const uint32_t *,
    const ospray::FeatureFlags &);
#define DECLARE_RENDERER_KERNEL_LAUNCHER(kernel_name)                          \
  namespace ispc {                                                             \
  extern "C" {                                                                 \
  void kernel_name(void *,                                                     \
      void *,                                                                  \
      const vec3ui &,                                                          \
      void *,                                                                  \
      void *,                                                                  \
      void *,                                                                  \
      void *,                                                                  \
      const uint32_t *,                                                        \
      const ospray::FeatureFlags &);                                           \
  }                                                                            \
  } // namespace ispc

// FrameOp kernel type
using FrameOpKernel = void (*)(void *, void *, const vec2ui &, const void *);
#define DECLARE_FRAMEOP_KERNEL_LAUNCHER(kernel_name)                           \
  namespace ispc {                                                             \
  extern "C" {                                                                 \
  void kernel_name(void *, void *, const vec2ui &, const void *);              \
  }                                                                            \
  } // namespace ispc

/////////////////////////////////////////////////////////////////////
// AsyncEvent interface

// Async event interface for polymorphism
struct DeviceImpl;
struct OSPRAY_CORE_INTERFACE AsyncEventIfc
{
  // Virtual destruction for polymorphism
  virtual ~AsyncEventIfc() = default;

  // Same interface as AsyncEvent
  virtual void wait() const = 0;
  virtual bool finished() const = 0;
  virtual float getDuration() const = 0;
  virtual void *getSyclEventPtr() = 0;

  // DeviceImpl class acts on this class
  friend struct DeviceImpl;
};

// Copyable async event class
struct OSPRAY_CORE_INTERFACE AsyncEvent
{
  // Construction & destruction
  AsyncEvent() = default;
  AsyncEvent(std::shared_ptr<AsyncEventIfc> ifc);

  // Wait for the event to complete
  void wait() const;

  // Return true if event finished
  bool finished() const;

  // Get event execution time
  float getDuration() const;

  // For code (e.g. OIDN) that is able to use SYCL without including it
  void *getSyclEventPtr();

 private:
  std::shared_ptr<AsyncEventIfc> ifc;
};

inline AsyncEvent::AsyncEvent(std::shared_ptr<AsyncEventIfc> ifc) : ifc(ifc) {}

inline void AsyncEvent::wait() const
{
  if (ifc)
    ifc->wait();
}

inline bool AsyncEvent::finished() const
{
  return ifc ? ifc->finished() : true;
}

inline float AsyncEvent::getDuration() const
{
  return ifc ? ifc->getDuration() : 0.f;
}

inline void *AsyncEvent::getSyclEventPtr()
{
  return ifc ? ifc->getSyclEventPtr() : nullptr;
}

/////////////////////////////////////////////////////////////////////
// Device interface

// The class representing a compute device like CPU or GPU
struct OSPRAY_CORE_INTERFACE Device
{
  // Construction & polymorphic destruction
  Device(bool debug) : debug(debug) {}
  virtual ~Device() = default;

  // Allocate device local memory
  virtual void *deviceMalloc(std::size_t size) = 0;

  // Allocate device shared memory
  virtual void *sharedMalloc(std::size_t size) = 0;

  // Allocate host memory
  virtual void *hostMalloc(std::size_t size) = 0;

  // Release memory allocated with any of the above malloc functions
  virtual void free(void *ptr) = 0;

  // Get memory allocation type for given pointer
  virtual Alloc getPointerType(void *ptr) const = 0;

  // Wait for device to finish its pending work
  virtual void wait() = 0;

  // Create asynchronous event object
  virtual AsyncEvent createAsyncEvent() = 0;

  // Copy memory from/to and within the device
  virtual AsyncEvent memcpy(void *dest, const void *src, std::size_t size) = 0;

  // Launch a kernel on a device
  virtual AsyncEvent launchRendererKernel(const vec3ui &itemDims,
      RendererKernel kernel,
      ispc::Renderer *renderer,
      ispc::FrameBuffer *fb,
      ispc::Camera *camera,
      ispc::World *world,
      const uint32_t *taskIDs,
      const FeatureFlags &ff) = 0;
  virtual AsyncEvent launchFrameOpKernel(const vec2ui &itemDims,
      FrameOpKernel kernel,
      const ispc::FrameBufferView *fbv) = 0;

  // Launch a given task on a host using device queue ordering
  virtual AsyncEvent launchHostTask(const std::function<void()> &task) = 0;

  // For code (e.g. OIDN) that is able to use SYCL without including it
  virtual void *getSyclDevicePtr() = 0;
  virtual void *getSyclContextPtr() = 0;
  virtual void *getSyclQueuePtr() = 0;

  // Check if device is in debug mode
  bool isDebug() const;

 private:
  bool debug;
};

inline bool Device::isDebug() const
{
  return debug;
}

/////////////////////////////////////////////////////////////////////
// Device allocator

template <typename T>
struct DeviceAllocator
{
  using value_type = T;

  // Construction
  DeviceAllocator(Device &device, Alloc alloc) : device(device), alloc(alloc) {}
  template <typename U>
  DeviceAllocator(const DeviceAllocator<U> &other)
      : device(other.device), alloc(other.alloc)
  {}

  // Memory allocation and release
  T *allocate(std::size_t n, const void *hint = 0);
  void deallocate(T *p, std::size_t n);

  // Check if it is the same allocator
  bool operator==(const DeviceAllocator &other) const;
  bool operator!=(const DeviceAllocator &other) const;

  // Get device used for allocation
  Device &getDevice() const;

 private:
  Device &device;
  Alloc alloc;

  // Provide access to private data for the templated copy constructor
  template <typename U>
  friend class DeviceAllocator;
};

template <typename T>
T *DeviceAllocator<T>::allocate(std::size_t n, const void *)
{
  return static_cast<T *>((alloc == Alloc::Shared)
          ? device.sharedMalloc(n * sizeof(T))
          : device.hostMalloc(n * sizeof(T)));
}

template <typename T>
void DeviceAllocator<T>::deallocate(T *p, std::size_t)
{
  device.free(p);
}

template <typename T>
bool DeviceAllocator<T>::operator==(const DeviceAllocator &other) const
{
  return &device == &other.device;
}

template <typename T>
bool DeviceAllocator<T>::operator!=(const DeviceAllocator &other) const
{
  return &device != &other.device;
}

template <typename T>
inline Device &DeviceAllocator<T>::getDevice() const
{
  return device;
}

/////////////////////////////////////////////////////////////////////
// Buffer in the device memory

// Use it for data that needs the highest read/write performance on the device.
// The buffer supports initialization from system (non-USM) memory. Preferably
// should be used for all scene data that does not need to be updated from or
// transferred back to the host.
template <typename T>
struct BufferDevice
{
  // Construct an uninitialized buffer in device memory
  BufferDevice(Device &device, std::size_t count);

  // Construct a buffer in device memory and initialize it with given vector
  BufferDevice(Device &device, const std::vector<T> &v);

  // Construct a buffer in device memory and initialize it with raw memory
  BufferDevice(Device &device, T *data, std::size_t count);

  // Release device memory
  ~BufferDevice();

  // Get device that was used for allocation
  Device &getDevice() const;

  // Get number of container elements
  size_t size() const;

  // Get device memory pointer
  T *devicePtr();
  const T *devicePtr() const;

 private:
  Device &device;
  const size_t count;
  T *const ptr;
};

template <typename T>
BufferDevice<T>::BufferDevice(Device &device, std::size_t count)
    : device(device),
      count(count),
      ptr(static_cast<T *>(device.deviceMalloc(count * sizeof(T))))
{}

template <typename T>
BufferDevice<T>::BufferDevice(Device &device, const std::vector<T> &v)
    : device(device),
      count(v.size()),
      ptr(static_cast<T *>(device.deviceMalloc(v.size() * sizeof(T))))
{
  // Schedule buffer copying and wait till completed so the input resource can
  // be released immediately
  AsyncEvent event = device.memcpy(ptr, v.data(), v.size() * sizeof(T));
  event.wait();
}

template <typename T>
BufferDevice<T>::BufferDevice(Device &device, T *data, std::size_t count)
    : device(device),
      count(count),
      ptr(static_cast<T *>(device.deviceMalloc(count * sizeof(T))))
{
  // Schedule buffer copying and wait till completed so the input resource can
  // be released immediately
  AsyncEvent event = device.memcpy(ptr, data, count * sizeof(T));
  event.wait();
}

template <typename T>
BufferDevice<T>::~BufferDevice()
{
  device.free(ptr);
}

template <typename T>
inline Device &BufferDevice<T>::getDevice() const
{
  return device;
}

template <typename T>
inline size_t BufferDevice<T>::size() const
{
  return count;
}

template <typename T>
inline T *BufferDevice<T>::devicePtr()
{
  return ptr;
}

template <typename T>
inline const T *BufferDevice<T>::devicePtr() const
{
  return ptr;
}

template <typename T, typename... Args>
inline std::unique_ptr<BufferDevice<T>> make_buffer_device_unique(
    Args &&...args)
{
  return std::unique_ptr<BufferDevice<T>>(
      new BufferDevice<T>(std::forward<Args>(args)...));
}

/////////////////////////////////////////////////////////////////////
// Buffer in the device memory with host memory shadow

// For usages that need the highest read/write efficiency on the device, similar
// to `BufferDevice`, but additionally the data has to be transferred from/to
// host periodically, e.g. framebuffers. After construction and initialization
// of the host/shadow buffer, an explicit call to copyToDevice() is required so
// the data will be visible for the device. Beware, it may use pinned (not
// pageable) memory on the host so it is not suitable for huge buffers.
template <typename T>
struct BufferDeviceShadowed : public std::vector<T, DeviceAllocator<T>>
{
  // Polymorfic destruction
  virtual ~BufferDeviceShadowed() = default;

  // Get the device that was used for allocation
  Device &getDevice() const;

  // Copy the host (shadow) buffer to the device
  AsyncEvent copyToDevice();

  // Copy the device buffer to the host
  AsyncEvent copyToHost();

  // Get host memory pointer
  T *hostPtr();
  const T *hostPtr() const;

  // Get device memory pointer
  T *devicePtr();
  const T *devicePtr() const;

 protected:
  // To be called by an implementation class
  BufferDeviceShadowed(Device &device, T *ptr, std::size_t count);
  BufferDeviceShadowed(Device &device, T *ptr, const std::vector<T> &v);
  BufferDeviceShadowed(Device &device, T *ptr, T *data, std::size_t count);

  // Copy between device and shadow buffers
  virtual AsyncEvent memcpy(T *dest, const T *src, std::size_t count) = 0;

  // Common members for all implementations
  Device &device;
  T *const devPtr;
};

template <typename T>
BufferDeviceShadowed<T>::BufferDeviceShadowed(
    Device &device, T *ptr, std::size_t count)
    : std::vector<T, DeviceAllocator<T>>(
        DeviceAllocator<T>(device, Alloc::Host)),
      device(device),
      devPtr(ptr)
{
  std::vector<T, DeviceAllocator<T>>::resize(
      count); // TODO: since C++14 use vector(count, allocator) constructor
}

template <typename T>
BufferDeviceShadowed<T>::BufferDeviceShadowed(
    Device &device, T *ptr, const std::vector<T> &v)
    : std::vector<T, DeviceAllocator<T>>(
        v.begin(), v.end(), DeviceAllocator<T>(device, Alloc::Host)),
      device(device),
      devPtr(ptr)
{}

template <typename T>
BufferDeviceShadowed<T>::BufferDeviceShadowed(
    Device &device, T *ptr, T *data, std::size_t count)
    : std::vector<T, DeviceAllocator<T>>(
        data, data + count, DeviceAllocator<T>(device, Alloc::Host)),
      device(device),
      devPtr(ptr)
{}

template <typename T>
Device &BufferDeviceShadowed<T>::getDevice() const
{
  return device;
}

template <typename T>
AsyncEvent BufferDeviceShadowed<T>::copyToDevice()
{
  return memcpy(devPtr,
      std::vector<T, DeviceAllocator<T>>::data(),
      std::vector<T, DeviceAllocator<T>>::size());
}

template <typename T>
AsyncEvent BufferDeviceShadowed<T>::copyToHost()
{
  return memcpy(std::vector<T, DeviceAllocator<T>>::data(),
      devPtr,
      std::vector<T, DeviceAllocator<T>>::size());
}

template <typename T>
inline T *BufferDeviceShadowed<T>::hostPtr()
{
  return std::vector<T, DeviceAllocator<T>>::data();
}

template <typename T>
inline const T *BufferDeviceShadowed<T>::hostPtr() const
{
  return std::vector<T, DeviceAllocator<T>>::data();
}

template <typename T>
inline T *BufferDeviceShadowed<T>::devicePtr()
{
  return devPtr ? devPtr : std::vector<T, DeviceAllocator<T>>::data();
}

template <typename T>
inline const T *BufferDeviceShadowed<T>::devicePtr() const
{
  return devPtr ? devPtr : std::vector<T, DeviceAllocator<T>>::data();
}

/////////////////////////////////////////////////////////////////////
// Buffer in the device shared memory

// For all other usages where simplicity of use is more important than
// performance.
template <typename T>
struct BufferShared : public std::vector<T, DeviceAllocator<T>>
{
  // Construct an uninitialized buffer in device shared memory
  BufferShared(Device &device, std::size_t count);

  // Construct a buffer in device shared memory and initialize it with given
  // vector
  BufferShared(Device &device, const std::vector<T> &v);

  // Construct a buffer in device shared memory and initialize it with raw
  // memory
  BufferShared(Device &device, const T *data, std::size_t count);

  // Get device that was used for allocation
  Device &getDevice() const;

  // Get shared memory pointer
  T *sharedPtr();
  const T *sharedPtr() const;
};

template <typename T>
BufferShared<T>::BufferShared(Device &device, std::size_t count)
    : std::vector<T, DeviceAllocator<T>>(
        DeviceAllocator<T>(device, Alloc::Shared))
{
  std::vector<T, DeviceAllocator<T>>::resize(
      count); // TODO: since C++14 use vector(count, allocator) constructor
}

template <typename T>
BufferShared<T>::BufferShared(Device &device, const std::vector<T> &v)
    : std::vector<T, DeviceAllocator<T>>(
        v.begin(), v.end(), DeviceAllocator<T>(device, Alloc::Shared))
{}

template <typename T>
BufferShared<T>::BufferShared(Device &device, const T *data, std::size_t count)
    : std::vector<T, DeviceAllocator<T>>(
        data, data + count, DeviceAllocator<T>(device, Alloc::Shared))
{}

template <typename T>
inline Device &BufferShared<T>::getDevice() const
{
  return std::vector<T, DeviceAllocator<T>>::get_allocator().getDevice();
}

template <typename T>
inline T *BufferShared<T>::sharedPtr()
{
  return std::vector<T, DeviceAllocator<T>>::data();
}

template <typename T>
inline const T *BufferShared<T>::sharedPtr() const
{
  return std::vector<T, DeviceAllocator<T>>::data();
}

template <typename T, typename... Args>
inline std::unique_ptr<BufferShared<T>> make_buffer_shared_unique(
    Args &&...args)
{
  return std::unique_ptr<BufferShared<T>>(
      new BufferShared<T>(std::forward<Args>(args)...));
}

} // namespace devicert

// Convenience type aliases
template <typename T>
using BufferDeviceUq = std::unique_ptr<devicert::BufferDevice<T>>;
template <typename T>
using BufferDeviceShadowedUq =
    std::unique_ptr<devicert::BufferDeviceShadowed<T>>;
template <typename T>
using BufferSharedUq = std::unique_ptr<devicert::BufferShared<T>>;

} // namespace ospray
