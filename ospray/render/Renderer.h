// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#include "texture/Texture2D.h"

namespace ospray {

/*! \brief abstract base class for all ospray renderers.

  \detailed Tthis base renderer abstraction only knows about
  'rendering a frame'; most actual renderers will be derived from a
  tile renderer, but this abstraction level also allows for frame
  compositing or even projection/splatting based approaches
 */
struct OSPRAY_SDK_INTERFACE Renderer : public ManagedObject
{
  Renderer();
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
  virtual float renderFrame(FrameBuffer *fb, Camera *camera, World *world);

  //! \brief called to initialize a new frame
  /*! this function gets called exactly once (on each node) at the
    beginning of each frame, and allows the renderer to do whatever
    is required to initialize a new frame. In particular, this
    function _can_ return a pointer to some "per-frame-data"; this
    pointer (can be NULL) is then passed to 'renderFrame' and
    'endFrame' to do with as they please

    \returns pointer to per-frame data, or NULL if this does not apply
   */
  virtual void *beginFrame(FrameBuffer *fb, World *world);

  /*! \brief called exactly once (on each node) at the end of each frame */
  virtual void endFrame(FrameBuffer *fb, void *perFrameData);

  /*! \brief called by the load balancer to render one tile of "samples" */
  virtual void renderTile(FrameBuffer *fb,
      Camera *camera,
      World *world,
      void *perFrameData,
      Tile &tile,
      size_t jobID) const;

  virtual OSPPickResult pick(
      FrameBuffer *fb, Camera *camera, World *world, const vec2f &screenPos);

  // Data //

  int32 spp{1};
  float errorThreshold{0.f};
  vec4f bgColor{0.f};

  Ref<Texture2D> maxDepthTexture;
  Ref<Texture2D> backplate;

  Ref<const DataT<Material *>> materialData;
  std::vector<void *> ispcMaterialPtrs;
};

OSPTYPEFOR_SPECIALIZATION(Renderer *, OSP_RENDERER);

/*! \brief registers a internal ospray::<ClassName> renderer under
    the externally accessible name "external_name"

    \internal This currently works by defining a extern "C" function
    with a given predefined name that creates a new instance of this
    renderer. By having this symbol in the shared lib ospray can
    lateron always get a handle to this fct and create an instance
    of this renderer.
*/
#define OSP_REGISTER_RENDERER(InternalClass, external_name)                    \
  OSP_REGISTER_OBJECT(                                                         \
      ::ospray::Renderer, renderer, InternalClass, external_name)

// Inlined definitions //////////////////////////////////////////////////////

inline void *Renderer::beginFrame(FrameBuffer *, World *)
{
  return nullptr;
}

inline void Renderer::endFrame(FrameBuffer *, void *) {}

} // namespace ospray
