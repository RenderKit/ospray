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

#include <unordered_map>
#include <vector>

#include <ospray/ospray.h>
#include "MPICommon.h"
#include "common/ObjectHandle.h"

#include "ospcommon/networking/DataStreaming.h"
#include "ospcommon/networking/Fabric.h"
#include "ospcommon/utility/ArrayView.h"

#include "camera/Camera.h"
#include "common/Instance.h"
#include "common/World.h"
#include "fb/ImageOp.h"
#include "geometry/Geometry.h"
#include "lights/Light.h"
#include "render/Renderer.h"
#include "volume/Volume.h"
#include "volume/transferFunction/TransferFunction.h"

namespace ospray {
namespace mpi {
namespace work {

using namespace ospcommon;

enum TAG
{
  NONE,
  NEW_RENDERER,
  NEW_WORLD,
  NEW_GEOMETRY,
  NEW_GEOMETRIC_MODEL,
  NEW_VOLUME,
  NEW_VOLUMETRIC_MODEL,
  NEW_CAMERA,
  NEW_TRANSFER_FUNCTION,
  NEW_IMAGE_OPERATION,
  NEW_MATERIAL,
  NEW_LIGHT,
  NEW_SHARED_DATA,
  NEW_DATA,
  COPY_DATA,
  NEW_TEXTURE,
  NEW_GROUP,
  NEW_INSTANCE,
  COMMIT,
  RELEASE,
  RETAIN,
  LOAD_MODULE,
  CREATE_FRAMEBUFFER,
  MAP_FRAMEBUFFER,
  GET_VARIANCE,
  RESET_ACCUMULATION,
  RENDER_FRAME_ASYNC,
  SET_PARAM_OBJECT,
  SET_PARAM_STRING,
  SET_PARAM_INT,
  SET_PARAM_BOOL,
  SET_PARAM_FLOAT,
  SET_PARAM_VEC2F,
  SET_PARAM_VEC2I,
  SET_PARAM_VEC3F,
  SET_PARAM_VEC3I,
  SET_PARAM_VEC4F,
  SET_PARAM_VEC4I,
  SET_PARAM_BOX1F,
  SET_PARAM_BOX1I,
  SET_PARAM_BOX2F,
  SET_PARAM_BOX2I,
  SET_PARAM_BOX3F,
  SET_PARAM_BOX3I,
  SET_PARAM_BOX4F,
  SET_PARAM_BOX4I,
  SET_PARAM_LINEAR3F,
  SET_PARAM_AFFINE3F,
  REMOVE_PARAM,
  SET_LOAD_BALANCER,
  PICK,
  FUTURE_IS_READY,
  FUTURE_WAIT,
  FUTURE_CANCEL,
  FUTURE_GET_PROGRESS,
  FINALIZE
};

struct FrameBufferInfo
{
  vec2i size = vec2i(0, 0);
  OSPFrameBufferFormat format = OSP_FB_NONE;
  uint32_t channels = 0;

  FrameBufferInfo() = default;
  FrameBufferInfo(
      const vec2i &size, OSPFrameBufferFormat format, uint32_t channels);

  size_t pixelSize(uint32_t channel) const;
  size_t getNumPixels() const;
};

struct OSPState
{
  std::unordered_map<int64_t, OSPObject> objects;

  std::unordered_map<int64_t, FrameBufferInfo> framebuffers;

  std::unordered_map<int64_t,
      std::unique_ptr<ospcommon::utility::OwnedArray<uint8_t>>>
      data;

  template <typename T>
  T getObject(int64_t handle)
  {
    return reinterpret_cast<T>(objects[handle]);
  }
};

void dispatchWork(TAG t,
    OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric);

const char *tagName(work::TAG t);
} // namespace work
} // namespace mpi
} // namespace ospray
