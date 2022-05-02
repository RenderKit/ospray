// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pf/PixelFilter.h"
#include "rkcommon/utility/ArrayView.h"
#include "texture/Texture2D.h"
// ispc shared
#include "RendererShared.h"

namespace ospray {

struct Camera;
struct World;
struct Material;
struct FrameBuffer;
struct PixelFilter;

// abstract base class for all ospray renderers.
//
// This base renderer abstraction only knows about
// 'rendering a frame'; most actual renderers will be derived from a
// tile renderer, but this abstraction level also allows for frame
// compositing or even projection/splatting based approaches
struct OSPRAY_SDK_INTERFACE Renderer
    : public AddStructShared<ManagedObject, ispc::Renderer>
{
  Renderer();
  virtual ~Renderer() override = default;

  static Renderer *createInstance(const char *identifier);
  template <typename T>
  static void registerType(const char *type);

  virtual void commit() override;
  virtual std::string toString() const override;

  // called to initialize a new frame
  //
  // this function gets called exactly once (on each node) at the
  // beginning of each frame, and allows the renderer to do whatever
  // is required to initialize a new frame. In particular, this
  // function _can_ return a pointer to some "per-frame-data"; this
  // pointer (can be NULL) is then passed to 'renderFrame' and
  // 'endFrame' to do with as they please
  //
  // returns pointer to per-frame data, or NULL if this does not apply
  virtual void *beginFrame(FrameBuffer *fb, World *world);

  // called exactly once (on each node) at the end of each frame
  virtual void endFrame(FrameBuffer *fb, void *perFrameData);

  // called by the load balancer to render one "sample" for each task
  virtual void renderTasks(FrameBuffer *fb,
      Camera *camera,
      World *world,
      void *perFrameData,
      const utility::ArrayView<uint32_t> &taskIDs) const;

  virtual OSPPickResult pick(
      FrameBuffer *fb, Camera *camera, World *world, const vec2f &screenPos);

  // Data //

  int32 spp{1};
  float errorThreshold{0.f};
  vec4f bgColor{0.f};

  Ref<Texture2D> maxDepthTexture;
  Ref<Texture2D> backplate;

  std::unique_ptr<PixelFilter> pixelFilter;

  Ref<const DataT<Material *>> materialData;
  std::vector<ispc::Material *> ispcMaterialPtrs;

 private:
  template <typename BASE_CLASS, typename CHILD_CLASS>
  friend void registerTypeHelper(const char *type);
  static void registerType(const char *type, FactoryFcn<Renderer> f);
  void setupPixelFilter();
};

OSPTYPEFOR_SPECIALIZATION(Renderer *, OSP_RENDERER);

// Inlined definitions ////////////////////////////////////////////////////////

template <typename T>
inline void Renderer::registerType(const char *type)
{
  registerTypeHelper<Renderer, T>(type);
}

inline void *Renderer::beginFrame(FrameBuffer *, World *)
{
  return nullptr;
}

inline void Renderer::endFrame(FrameBuffer *, void *) {}

} // namespace ospray
