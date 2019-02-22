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

#include <vector>

#include <ospray/ospray_cpp/ManagedObject.h>

namespace ospray {
namespace cpp    {

struct AMRBrickInfo
{
  ospcommon::box3i bounds;
  int              refinemntLevel;
  float            cellWidth;
};

class Volume : public ManagedObject_T<OSPVolume>
{
public:

  Volume(const std::string &type);
  Volume(const Volume &copy);
  Volume(OSPVolume existing);

  void setRegion(void *source,
                 const ospcommon::vec3i &regionCoords,
                 const ospcommon::vec3i &regionSize) const;
};

// Inlined function definitions ///////////////////////////////////////////////

inline Volume::Volume(const std::string &type)
{
  OSPVolume c = ospNewVolume(type.c_str());
  if (c) {
    ospObject = c;
  } else {
    throw std::runtime_error("Failed to create OSPVolume!");
  }
}

inline Volume::Volume(const Volume &copy) :
  ManagedObject_T<OSPVolume>(copy.handle())
{
}

inline Volume::Volume(OSPVolume existing) :
  ManagedObject_T<OSPVolume>(existing)
{
}

inline void Volume::setRegion(void *source,
                              const ospcommon::vec3i &regionCoords,
                              const ospcommon::vec3i &regionSize) const
{
  // TODO return error code
  ospSetRegion(handle(),
               source,
               (const osp_vec3i&)regionCoords,
               (const osp_vec3i&)regionSize);
}

}// namespace cpp
}// namespace ospray
