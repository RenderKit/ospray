// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "ospray/ospray.h"

#include <stdexcept>
#include <string>

namespace ospray {
namespace cpp    {

class Device
{
public:

  Device(const std::string &type = "default");
  Device(const Device &copy);
  Device(OSPDevice existing);

  void set(const std::string &name, const std::string &v) const;
  void set(const std::string &name, int v) const;
  void set(const std::string &name, void *v) const;

  void commit() const;

  void setCurrent() const;

  OSPDevice handle() const;

private:

  OSPDevice ospHandle;
};

// Inlined function definitions ///////////////////////////////////////////////

inline Device::Device(const std::string &type)
{
  OSPDevice c = ospNewDevice(type.c_str());
  if (c) {
    ospHandle = c;
  } else {
    throw std::runtime_error("Failed to create OSPDevice!");
  }
}

inline Device::Device(const Device &copy) :
  ospHandle(copy.ospHandle)
{
}

inline Device::Device(OSPDevice existing) :
  ospHandle(existing)
{
}

inline void Device::set(const std::string &name, const std::string &v) const
{
  ospDeviceSetString(ospHandle, name.c_str(), v.c_str());
}

inline void Device::set(const std::string &name, int v) const
{
  ospDeviceSet1i(ospHandle, name.c_str(), v);
}

inline void Device::set(const std::string &name, void *v) const
{
  ospDeviceSetVoidPtr(ospHandle, name.c_str(), v);
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

}// namespace cpp
}// namespace ospray
