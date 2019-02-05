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

  void sampleVolume(float **results,
                    const ospcommon::vec3f *worldCoordinates,
                    size_t count) const;
  std::vector<float> sampleVolume(const std::vector<ospcommon::vec3f> &points) const;
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
               (const osp::vec3i&)regionCoords,
               (const osp::vec3i&)regionSize);
}

inline void Volume::sampleVolume(float **results,
                                 const ospcommon::vec3f *worldCoordinates,
                                 size_t count) const
{
  ospSampleVolume(results,
                  handle(),
                  (const osp::vec3f&)*worldCoordinates,
                  count);
}

inline std::vector<float>
Volume::sampleVolume(const std::vector<ospcommon::vec3f> &points) const
{
  float *results = nullptr;
  auto numPoints = points.size();
  sampleVolume(&results, points.data(), numPoints);

  if (!results)
    throw std::runtime_error("Failed to sample volume!");

  std::vector<float> retval(points.size());
  memcpy(retval.data(), results, numPoints*sizeof(float));
  return retval;
}

}// namespace cpp
}// namespace ospray
