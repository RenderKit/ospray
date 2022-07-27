// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospray/ospray.h"

#include <stdexcept>
#include <string>

namespace ospray {
namespace cpp {

class Device
{
 public:
  Device(const std::string &type = "cpu");
  Device(const Device &copy);
  Device(Device &&move);
  Device(OSPDevice existing = nullptr);
  ~Device();

  Device &operator=(const Device &copy);
  Device &operator=(Device &&move);

  template <typename FCN_T>
  void setErrorCallback(FCN_T &&fcn);

  template <typename FCN_T>
  void setStatusCallback(FCN_T &&fcn);

  template <typename T>
  void setParam(const std::string &name, const T &v) const;

  void setParam(const std::string &name, const char *v) const;
  void setParam(const std::string &name, const std::string &v) const;

  void setParam(const std::string &name, OSPDataType, const void *) const;

  void removeParam(const std::string &name) const;

  void commit() const;

  void setCurrent() const;

  static Device current();

  OSPDevice handle() const;

 private:
  OSPDevice ospHandle;
};

// Inlined function definitions ///////////////////////////////////////////

inline Device::Device(const std::string &type)
{
  ospHandle = ospNewDevice(type.c_str());
}

inline Device::Device(const Device &copy) : ospHandle(copy.ospHandle)
{
  ospDeviceRetain(copy.handle());
}

inline Device::Device(Device &&move)
{
  ospHandle = move.handle();
  move.ospHandle = nullptr;
}

inline Device::Device(OSPDevice existing) : ospHandle(existing) {}

inline Device::~Device()
{
  ospDeviceRelease(ospHandle);
}

inline Device &Device::operator=(const Device &copy)
{
  ospDeviceRelease(ospHandle);
  ospHandle = copy.handle();
  ospDeviceRetain(copy.handle());
  return *this;
}

inline Device &Device::operator=(Device &&move)
{
  ospDeviceRelease(ospHandle);
  ospHandle = move.handle();
  move.ospHandle = nullptr;
  return *this;
}

template <typename FCN_T>
inline void Device::setErrorCallback(FCN_T &&fcn)
{
  ospDeviceSetErrorCallback(ospHandle, std::forward<FCN_T>(fcn), nullptr);
}

template <typename FCN_T>
inline void Device::setStatusCallback(FCN_T &&fcn)
{
  ospDeviceSetStatusCallback(ospHandle, std::forward<FCN_T>(fcn), nullptr);
}

template <typename T>
inline void Device::setParam(const std::string &name, const T &v) const
{
  static_assert(OSPTypeFor<T>::value != OSP_UNKNOWN,
      "Only types corresponding to OSPDataType values can be set "
      "as parameters on OSPRay objects. NOTE: Math types (vec, "
      "box, linear, affine) are "
      "expected to come from rkcommon::math.");
  ospDeviceSetParam(ospHandle, name.c_str(), OSPTypeFor<T>::value, &v);
}

inline void Device::setParam(const std::string &name, const char *v) const
{
  ospDeviceSetParam(ospHandle, name.c_str(), OSP_STRING, v);
}

inline void Device::setParam(
    const std::string &name, const std::string &v) const
{
  ospDeviceSetParam(ospHandle, name.c_str(), OSP_STRING, v.c_str());
}

inline void Device::setParam(
    const std::string &name, OSPDataType type, const void *mem) const
{
  ospDeviceSetParam(ospHandle, name.c_str(), type, mem);
}

inline void Device::removeParam(const std::string &name) const
{
  ospDeviceRemoveParam(ospHandle, name.c_str());
}

inline void Device::commit() const
{
  ospDeviceCommit(ospHandle);
}

inline void Device::setCurrent() const
{
  ospSetCurrentDevice(ospHandle);
}

inline Device Device::current()
{
  return Device(ospGetCurrentDevice());
}

inline OSPDevice Device::handle() const
{
  return ospHandle;
}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Device, OSP_DEVICE);

} // namespace ospray
