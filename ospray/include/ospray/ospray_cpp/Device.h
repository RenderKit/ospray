// Copyright 2009-2019 Intel Corporation
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
  Device(OSPDevice existing = nullptr);

  ~Device();

  void set(const std::string &name, const std::string &v) const;
  void set(const std::string &name, bool v) const;
  void set(const std::string &name, int v) const;
  void set(const std::string &name, void *v) const;

  void commit() const;

  void setCurrent() const;

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

inline Device::Device(OSPDevice existing) : ospHandle(existing) {}

inline Device::~Device()
{
  ospDeviceRelease(ospHandle);
}

inline void Device::set(const std::string &name, const std::string &v) const
{
  auto *s = v.c_str();
  ospDeviceSetParam(ospHandle, name.c_str(), OSP_STRING, &s);
}

inline void Device::set(const std::string &name, bool v) const
{
  ospDeviceSetParam(ospHandle, name.c_str(), OSP_BOOL, &v);
}

inline void Device::set(const std::string &name, int v) const
{
  ospDeviceSetParam(ospHandle, name.c_str(), OSP_INT, &v);
}

inline void Device::set(const std::string &name, void *v) const
{
  ospDeviceSetParam(ospHandle, name.c_str(), OSP_VOID_PTR, &v);
}

inline void Device::commit() const
{
  ospDeviceCommit(ospHandle);
}

inline void Device::setCurrent() const
{
  ospSetCurrentDevice(ospHandle);
}

inline OSPDevice Device::handle() const
{
  return ospHandle;
}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Device, OSP_DEVICE);

} // namespace ospray
