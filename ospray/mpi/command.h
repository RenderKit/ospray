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

/*! \file ospray/device/nwlayer.h \brief Defines the basic network layer abstraction */

#include "include/ospray/ospray.h"
#include "mpi/buffers.h"

namespace ospray {
  // namespace nwlayer {

  /*! the command ID is a numerical value that corresponds to a
    given command type, and that lets the receiver figure out what
    kind of command it is to execute. CommandIDs are only useful
    inside \see Command structures. */
  typedef enum {
    CMD_NEW_RENDERER=0,
    CMD_FRAMEBUFFER_CREATE,
    CMD_RENDER_FRAME,
    CMD_FRAMEBUFFER_CLEAR,
    CMD_FRAMEBUFFER_MAP,
    CMD_FRAMEBUFFER_UNMAP,

    CMD_NEW_DATA = 100,
    CMD_NEW_MODEL,
    CMD_NEW_GEOMETRY,
    CMD_NEW_MATERIAL,
    CMD_NEW_LIGHT,
    CMD_NEW_TRIANGLEMESH,
    CMD_NEW_CAMERA,
    CMD_NEW_VOLUME,
    CMD_NEW_TRANSFERFUNCTION,
    CMD_NEW_TEXTURE2D,

    CMD_ADD_GEOMETRY=200,
    CMD_REMOVE_GEOMETRY,
    CMD_ADD_VOLUME,
    CMD_COMMIT,
    CMD_LOAD_MODULE,
    CMD_RELEASE,
    CMD_REMOVE_VOLUME,

    CMD_GET_TYPE=300,
    CMD_GET_VALUE,
    CMD_SET_MATERIAL,
    CMD_SET_REGION,
    CMD_SET_OBJECT,
    CMD_SET_STRING,
    CMD_SET_INT,
    CMD_SET_FLOAT,
    CMD_SET_VEC2F,
    CMD_SET_VEC2I,
    CMD_SET_VEC3F,
    CMD_SET_VEC3I,
    CMD_SET_VEC4F,

    CMD_SET_PIXELOP,
    CMD_NEW_PIXELOP,

    CMD_API_MODE,
    CMD_SAMPLE_VOLUME,

    CMD_FINALIZE,

    CMD_USER
  } CommandTag;

  namespace nwlayer {
    struct InitCmd {
    };
    struct NewFrameBufferCmd {
      vec2ui size;
      OSPFrameBufferFormat externalFormat;
      int channelFlags;
    };

  } // ::ospray::nwlayer
} // ::ospray
