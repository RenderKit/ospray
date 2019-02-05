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

#include <ospray/ospray_cpp/ManagedObject.h>

namespace ospray {
namespace cpp    {

class TransferFunction : public ManagedObject_T<OSPTransferFunction>
{
public:

  TransferFunction();
  TransferFunction(const std::string &type);
  TransferFunction(const TransferFunction &copy);
  TransferFunction(OSPTransferFunction existing);
};

// Inlined function definitions ///////////////////////////////////////////////

inline TransferFunction::TransferFunction() {}

inline TransferFunction::TransferFunction(const std::string &type)
{
  OSPTransferFunction c = ospNewTransferFunction(type.c_str());
  if (c) {
    ospObject = c;
  } else {
    throw std::runtime_error("Failed to create OSPTransferFunction!");
  }
}

inline TransferFunction::TransferFunction(const TransferFunction &copy) :
  ManagedObject_T<OSPTransferFunction>(copy.handle())
{
}

inline TransferFunction::TransferFunction(OSPTransferFunction existing) :
  ManagedObject_T<OSPTransferFunction>(existing)
{
}

}// namespace cpp
}// namespace ospray
