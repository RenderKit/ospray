// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "DebugRenderer.h"
#include "DebugRendererType.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc exports
#include "render/debug/DebugRenderer_ispc.h"
#else
#include "DebugRenderer.ih"
#include "render/RendererRenderTaskFn.inl"
#endif

namespace ospray {

// Helper functions /////////////////////////////////////////////////////////

static DebugRendererType typeFromString(const std::string &name)
{
  if (name == "rayDir")
    return DebugRendererType::RAY_DIR;
  else if (name == "eyeLight")
    return DebugRendererType::EYE_LIGHT;
  else if (name == "Ng")
    return DebugRendererType::NG;
  else if (name == "Ns")
    return DebugRendererType::NS;
  else if (name == "color")
    return DebugRendererType::COLOR;
  else if (name == "texCoord")
    return DebugRendererType::TEX_COORD;
  else if (name == "backfacing_Ng")
    return DebugRendererType::BACKFACING_NG;
  else if (name == "backfacing_Ns")
    return DebugRendererType::BACKFACING_NS;
  else if (name == "dPds")
    return DebugRendererType::DPDS;
  else if (name == "dPdt")
    return DebugRendererType::DPDT;
  else if (name == "primID")
    return DebugRendererType::PRIM_ID;
  else if (name == "geomID")
    return DebugRendererType::GEOM_ID;
  else if (name == "instID")
    return DebugRendererType::INST_ID;
  else if (name == "volume")
    return DebugRendererType::VOLUME;
  else
    return DebugRendererType::TEST_FRAME;
}

// DebugRenderer definitions ////////////////////////////////////////////////

DebugRenderer::DebugRenderer(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtDevice(), device)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.renderSample =
      reinterpret_cast<ispc::Renderer_RenderSampleFct>(
          ispc::DebugRenderer_testFrame_addr());
#endif
}

std::string DebugRenderer::toString() const
{
  return "ospray::DebugRenderer";
}

void DebugRenderer::commit()
{
  Renderer::commit();

  const std::string method = getParam<std::string>("method", "eyeLight");
  const auto debugType = typeFromString(method);
  getSh()->type = debugType;

#ifndef OSPRAY_TARGET_SYCL
  switch (debugType) {
  case RAY_DIR:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_rayDir_addr());
    break;
  case EYE_LIGHT:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_eyeLight_addr());
    break;
  case NG:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_Ng_addr());
    break;
  case NS:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_Ns_addr());
    break;
  case COLOR:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_vertexColor_addr());
    break;
  case TEX_COORD:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_texCoord_addr());
    break;
  case DPDS:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_dPds_addr());
    break;
  case DPDT:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_dPdt_addr());
    break;
  case PRIM_ID:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_primID_addr());
    break;
  case GEOM_ID:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_geomID_addr());
    break;
  case INST_ID:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_instID_addr());
    break;
  case BACKFACING_NG:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_backfacing_Ng_addr());
    break;
  case BACKFACING_NS:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_backfacing_Ns_addr());
    break;
  case VOLUME:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_volume_addr());
    break;
  case TEST_FRAME:
  default:
    getSh()->super.renderSample =
        reinterpret_cast<ispc::Renderer_RenderSampleFct>(
            ispc::DebugRenderer_testFrame_addr());
    break;
  }
#endif
}

#ifdef OSPRAY_TARGET_SYCL
void DebugRenderer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs,
    sycl::queue &syclQueue) const
{
  auto *rendererSh = getSh();
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const uint32_t *taskIDsPtr = taskIDs.data();
  const size_t numTasks = taskIDs.size();

  auto event = syclQueue.submit([&](sycl::handler &cgh) {
    const cl::sycl::nd_range<1> dispatchRange =
        computeDispatchRange(numTasks, RTC_SYCL_SIMD_WIDTH);
    cgh.parallel_for(dispatchRange, [=](cl::sycl::nd_item<1> taskIndex) {
      if (taskIndex.get_global_id(0) < numTasks) {
        ispc::Renderer_default_renderTask(&rendererSh->super,
            fbSh,
            cameraSh,
            worldSh,
            perFrameData,
            taskIDsPtr,
            taskIndex.get_global_id(0),
            ispc::DebugRenderer_renderSample);
      }
    });
  });
  event.wait_and_throw();
  // For prints we have to flush the entire queue, because other stuff is queued
  syclQueue.wait_and_throw();
}
#endif

} // namespace ospray
