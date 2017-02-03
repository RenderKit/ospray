// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

  Device(const std::string &type);
  Device(const Device &copy);
  Device(OSPDevice existing = nullptr);

  void set(const std::string &name, const std::string &v) const;
  void set(const std::string &name, int v) const;

  void commit() const;

  void setCurrent() const;

private:

  OSPDevice handle;
};

// Inlined function definitions ///////////////////////////////////////////////

inline Device::Device(const std::string &type)
{
  OSPDevice c = ospCreateDevice(type.c_str());
  if (c) {
    handle = c;
  } else {
    throw std::runtime_error("Failed to create OSPDevice!");
  }
}

inline Device::Device(const Device &copy) :
  handle(copy.handle)
{
}

inline Device::Device(OSPDevice existing) :
  handle(existing)
{
}

inline void Device::set(const std::string &name, const std::string &v) const
{
  ospDeviceSetString(handle, name.c_str(), v.c_str());
}

inline void Device::set(const std::string &name, int v) const
{
  ospDeviceSet1i(handle, name.c_str(), v);
}

inline void Device::commit() const
{
  ospDeviceCommit(handle);
}

inline void Device::setCurrent() const
{
  ospSetCurrentDevice(handle);
}

}// namespace cpp
}// namespace ospray
