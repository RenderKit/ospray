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

class Data : public ManagedObject_T<OSPData>
{
public:

  Data(size_t numItems, OSPDataType format,
       const void *init = nullptr, int flags = 0);
  Data(const Data &copy);
  Data(OSPData existing);
};

// Inlined function definitions ///////////////////////////////////////////////

inline Data::Data(size_t numItems, OSPDataType format,
                  const void *init, int flags)
{
  ospObject = ospNewData(numItems, format, init, flags);
}

inline Data::Data(const Data &copy) :
  ManagedObject_T<OSPData>(copy.handle())
{
}

inline Data::Data(OSPData existing) :
  ManagedObject_T<OSPData>(existing)
{
}

}// namespace cpp
}// namespace ospray
