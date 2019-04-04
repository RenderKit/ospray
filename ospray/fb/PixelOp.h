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

// ospray
#include "fb/Tile.h"
#include "common/Managed.h"

namespace ospray {

  struct FrameBuffer;

  /*! \brief base abstraction for a "Pixel Op" to be performed for
      every pixel that gets written into a frame buffer.

      A PixelOp is basically a "hook" that allows to inject arbitrary
      code, such as postprocessing, filtering, blending, tone mapping,
      sending tiles to a display wall, etc. PixelOps are intentionally
      'stateless' in that it they should be pure functors that can be
      applied to different frame buffers, potentially at the same
      time.

      To allow a pixelop to maintain some sort of state for a frame,
      the 'beginframe', a pixelop is supposed to create and return a
      state every time it gets "attached" to a frame buffer, and this
      state then gets passed every time a frame buffer gets started,
  */
  struct OSPRAY_SDK_INTERFACE PixelOp : public ManagedObject
  {
      virtual ~PixelOp() override = default;
      /*! gets called once at the beginning of the frame */
      virtual void beginFrame() {}
      /*! gets called once at the end of the frame */
      virtual void endFrame() {}

      /*! called right after the tile got accumulated; i.e., the
          tile's RGBA values already contain the accu-buffer blended
          values (assuming an accubuffer exists), and this function
          defines how these pixels are being processed before written
          into the color buffer */
      virtual void postAccum(FrameBuffer *, Tile &) {}

      //! \brief common function to help printf-debugging
      /*! Every derived class should override this! */
      virtual std::string toString() const override;

      static PixelOp *createInstance(const char *type);
  };

  /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      pixelop. By having this symbol in the shared lib ospray can
      later on always get a handle to this fct and create an instance
      of this pixelop.
  */
#define OSP_REGISTER_PIXEL_OP(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(PixelOp, pixel_op, InternalClass, external_name)

}
