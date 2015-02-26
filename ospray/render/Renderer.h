// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "ospray/common/Model.h"
#include "ospray/fb/FrameBuffer.h"

namespace ospray {

  struct Material;
  struct Light;

  /*! \brief abstract base class for all ospray renderers. 

    the base renderer only knows about 'rendering a frame'; most
    actual renderers will be derived from a tile renderer, but this
    abstraction level also allows for frame compositing or even
    projection/splatting based approaches
   */
  struct Renderer : public ManagedObject {
    uint32 spp;
    float  nearClip;
    Renderer() : spp(1), nearClip(1e-6f) {}
    virtual void commit();
    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::Renderer"; }
    /*! \brief produce one frame, and put it into given frame
        buffer */
    virtual void renderFrame(FrameBuffer *fb,
                             const uint32 fbChannelFlags);

    virtual void beginFrame(FrameBuffer *fb);
    virtual void endFrame(const int32 fbChannelFlags);
    virtual void renderTile(Tile &tile);
    
    /*! \brief create a material of given type */
    virtual Material *createMaterial(const char *type) { return NULL; }

    /*! \brief create a light of given type */
    virtual Light *createLight(const char *type) { return NULL; }

    virtual OSPPickData unproject(const vec2f &screenPos);

    FrameBuffer *currentFB;

    /*! \brief creates an abstract renderer class of given type 

      The respective renderer type must be a registered renderer type
      in either ospray proper or any already loaded module. For
      renderer types specified in special modules, make sure to call
      ospLoadModule first. */
    static Renderer *createRenderer(const char *identifier);
  };

  /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name" 
      
      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      renderer. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this renderer.
  */
#define OSP_REGISTER_RENDERER(InternalClassName,external_name)      \
  extern "C" Renderer *ospray_create_renderer__##external_name()    \
  {                                                                 \
    return new InternalClassName;                                   \
  }                                                                 \
  
} // ::ospray

