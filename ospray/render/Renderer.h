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

/*! \file Renderer.h Defines the base renderer class */

#include "common/Model.h"
#include "fb/FrameBuffer.h"
#include "texture/Texture2D.h"

namespace ospray {

  struct Material;
  struct Light;

  /*! \brief abstract base class for all ospray renderers.

    \detailed Tthis base renderer abstraction only knows about
    'rendering a frame'; most actual renderers will be derived from a
    tile renderer, but this abstraction level also allows for frame
    compositing or even projection/splatting based approaches
   */
  struct OSPRAY_SDK_INTERFACE Renderer : public ManagedObject
  {
    Renderer() = default;
    virtual ~Renderer() override = default;

    /*! \brief creates an abstract renderer class of given type

      The respective renderer type must be a registered renderer type
      in either ospray proper or any already loaded module. For
      renderer types specified in special modules, make sure to call
      ospLoadModule first. */
    static Renderer *createInstance(const char *identifier);

    virtual void commit() override;
    virtual std::string toString() const override;

    /*! \brief render one frame, and put it into given frame buffer */
    virtual float renderFrame(FrameBuffer *fb, const uint32 fbChannelFlags);

    //! \brief called to initialize a new frame
    /*! this function gets called exactly once (on each node) at the
      beginning of each frame, and allows the renderer to do whatever
      is required to initialize a new frame. In particular, this
      function _can_ return a pointer to some "per-frame-data"; this
      pointer (can be NULL) is then passed to 'renderFrame' and
      'endFrame' to do with as they please

      \returns pointer to per-frame data, or NULL if this does not apply
     */
    virtual void *beginFrame(FrameBuffer *fb);

    /*! \brief called exactly once (on each node) at the end of each frame */
    virtual void endFrame(void *perFrameData, const int32 fbChannelFlags);

    /*! \brief called by the load balancer to render one tile of "samples" */
    virtual void renderTile(void *perFrameData, Tile &tile, size_t jobID) const;

    virtual OSPPickResult pick(const vec2f &screenPos);

    Model *model {nullptr};
    FrameBuffer *currentFB {nullptr};

    /*! \brief number of samples to be used per pixel in a tile */
    int32 spp {1};

    /*! adaptive accumulation: variance-based error to reach */
    float errorThreshold {0.f};

    /*! \brief the background color */
    vec4f bgColor {0.f};

    /*! \brief maximum depth texture provided as an optional parameter to the renderer, used for early ray termination

      The texture format should be OSP_TEXTURE_R32F and texture filtering
      should be set to nearest-neighbor interpolation:
      (OSP_TEXTURE_FILTER_NEAREST). */
    Ref<Texture2D> maxDepthTexture;
  };

  /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      renderer. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this renderer.
  */
#define OSP_REGISTER_RENDERER(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(::ospray::Renderer, renderer, \
                      InternalClass, external_name)

} // ::ospray

