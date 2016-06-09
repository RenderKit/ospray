// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include <ospray_cpp/FrameBuffer.h>
#include <ospray_cpp/Light.h>
#include <ospray_cpp/ManagedObject.h>
#include <ospray_cpp/Material.h>

namespace ospray {
namespace cpp    {

class Renderer : public ManagedObject_T<OSPRenderer>
{
public:

  Renderer(const std::string &type);
  Renderer(const Renderer &copy);
  Renderer(OSPRenderer existing = nullptr);

  Material newMaterial(const std::string &type);
  Light    newLight(const std::string &type);

  void renderFrame(const FrameBuffer &fb, uint32_t channels);
};

// Inlined function definitions ///////////////////////////////////////////////

inline Renderer::Renderer(const std::string &type)
{
  OSPRenderer c = ospNewRenderer(type.c_str());
  if (c) {
    m_object = c;
  } else {
    throw std::runtime_error("Failed to create OSPRenderer!");
  }
}

inline Renderer::Renderer(const Renderer &copy) :
  ManagedObject_T(copy.handle())
{
}

inline Renderer::Renderer(OSPRenderer existing) :
  ManagedObject_T(existing)
{
}

inline Material Renderer::newMaterial(const std::string &type)
{
  auto mat = Material(ospNewMaterial(handle(), type.c_str()));

  if (!mat.handle()) {
    throw std::runtime_error("Failed to create OSPMaterial!");
  }

  return mat;
}

inline Light Renderer::newLight(const std::string &type)
{
  return Light(ospNewLight(handle(), type.c_str()));
}

inline void Renderer::renderFrame(const FrameBuffer &fb, uint32_t channels)
{
  ospRenderFrame(fb.handle(), handle(), channels);
}


}// namespace cpp
}// namespace ospray
