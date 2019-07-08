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

#include "common/Managed.h"
#include "fb/Tile.h"

namespace ospray {

  struct FrameBuffer;

  /*! \brief base abstraction for a "Image Op" to be performed for
      every image that gets written into a frame buffer.

      A ImageOp is basically a "hook" that allows to inject arbitrary
      code, such as postprocessing, filtering, blending, tone mapping,
      sending tiles to a display wall, etc.
  */
  struct OSPRAY_SDK_INTERFACE ImageOp : public ManagedObject
  {
      virtual ~ImageOp() override = default;
      /*! gets called once at the beginning of the frame */
      virtual void beginFrame() {}
      /*! gets called once at the end of the frame */
      virtual void endFrame() {}

      //! \brief common function to help printf-debugging
      /*! Every derived class should override this! */
      virtual std::string toString() const override;

      static ImageOp *createInstance(const char *type);
  };

#if 0
  /*! The pixel op is an imageop that works at the granularity
   * of a single pixel. The process method may be called in parallel
   * over multiple pixels.
   * TODO: Will it actually be useful to have these pixel ops? or just
   * leave it to users to decide how they want to run things in parallel
   * (or not) within a tile op?
  */
  struct OSPRAY_SDK_INTERFACE PixelOp : public ImageOp
  {
      virtual ~PixelOp() override = default;

      /*! called right after the tile got accumulated; i.e., the
          tile's RGBA values already contain the accu-buffer blended
          values (assuming an accubuffer exists), and this function
          defines how these pixels are being processed before written
          into the color buffer */
      //virtual void postAccum(FrameBuffer *, Tile &) {}

      //! \brief common function to help printf-debugging
      /*! Every derived class should override this! */
      virtual std::string toString() const override;
  };
#endif

  /*! The tile op is an imageop that works at the granularity
   * of a single tile. The process method may be called in parallel
   * over multiple tiles.
  */
  struct OSPRAY_SDK_INTERFACE TileOp : public ImageOp
  {
      virtual ~TileOp() override = default;

      /*! called right after the tile got accumulated; i.e., the
          tile's RGBA values already contain the accu-buffer blended
          values (assuming an accubuffer exists), and this function
          defines how these pixels are being processed before written
          into the color buffer */
      virtual void process(FrameBuffer *, Tile &) = 0;

      //! \brief common function to help printf-debugging
      /*! Every derived class should override this! */
      virtual std::string toString() const override;
  };

  /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      imageop. By having this symbol in the shared lib ospray can
      later on always get a handle to this fct and create an instance
      of this imageop.
  */
#define OSP_REGISTER_IMAGE_OP(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(ImageOp, image_op, InternalClass, external_name)
}

