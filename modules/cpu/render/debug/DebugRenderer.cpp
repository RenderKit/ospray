// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "DebugRenderer.h"
#include "DebugRendererType.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_DPCPP
// ispc exports
#include "render/debug/DebugRenderer_ispc.h"
#else
#include "DebugRenderer.ih"
#include "render/RendererRenderTaskFn.inl"

#if 0
// For DPCPP function pointers, later
namespace ispc {
#define ISPC_EXPORT_FN_DECL(name)                                              \
  RTC_SYCL_INDIRECTLY_CALLABLE void name(Renderer *uniform,                    \
      FrameBuffer *uniform fb,                                                 \
      World *uniform world,                                                    \
      void *uniform perFrameData,                                              \
      varying ScreenSample &sample);

ISPC_EXPORT_FN_DECL(DebugRenderer_testFrame);
ISPC_EXPORT_FN_DECL(DebugRenderer_rayDir);
ISPC_EXPORT_FN_DECL(DebugRenderer_eyeLight);
ISPC_EXPORT_FN_DECL(DebugRenderer_Ng);
ISPC_EXPORT_FN_DECL(DebugRenderer_Ns);
ISPC_EXPORT_FN_DECL(DebugRenderer_vertexColor);
ISPC_EXPORT_FN_DECL(DebugRenderer_texCoord);
ISPC_EXPORT_FN_DECL(DebugRenderer_dPds);
ISPC_EXPORT_FN_DECL(DebugRenderer_dPdt);
ISPC_EXPORT_FN_DECL(DebugRenderer_primID);
ISPC_EXPORT_FN_DECL(DebugRenderer_geomID);
ISPC_EXPORT_FN_DECL(DebugRenderer_instID);
ISPC_EXPORT_FN_DECL(DebugRenderer_backfacing_Ng);
ISPC_EXPORT_FN_DECL(DebugRenderer_backfacing_Ns);
ISPC_EXPORT_FN_DECL(DebugRenderer_volume);

#undef ISPC_EXPORT_FN_DECL
} // namespace ispc
#endif
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
#ifndef OSPRAY_TARGET_DPCPP
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

#ifndef OSPRAY_TARGET_DPCPP
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

#ifdef OSPRAY_TARGET_DPCPP
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

/*
void DebugRenderer::setGPUFunctionPtrs(sycl::queue &syclQueue)
{
  Renderer::setGPUFunctionPtrs(syclQueue);

  // TODO: This would need to be called on commit of the renderer
  // const std::string method = getParam<std::string>("method", "eyeLight");
  const std::string method = getParam<std::string>("method", "Ng");
  // const std::string method = getParam<std::string>("method", "testFrame");
  const DebugRendererType debugType = typeFromString(method);

  auto *sSh = getSh();
  auto event = syclQueue.submit([&](sycl::handler &cgh) {
    cgh.parallel_for(1, [=](cl::sycl::id<1>) RTC_SYCL_KERNEL {
      switch (debugType) {
      case RAY_DIR:
        sSh->renderSample = ispc::DebugRenderer_rayDir;
        break;
      case EYE_LIGHT:
        sSh->renderSample = ispc::DebugRenderer_eyeLight;
        break;
      case NG:
        sSh->renderSample = ispc::DebugRenderer_Ng;
        break;
      case NS:
        sSh->renderSample = ispc::DebugRenderer_Ns;
        break;
      case COLOR:
        sSh->renderSample = ispc::DebugRenderer_vertexColor;
        break;
      case TEX_COORD:
        sSh->renderSample = ispc::DebugRenderer_texCoord;
        break;
      case DPDS:
        sSh->renderSample = ispc::DebugRenderer_dPds;
        break;
      case DPDT:
        sSh->renderSample = ispc::DebugRenderer_dPdt;
        break;
      case PRIM_ID:
        sSh->renderSample = ispc::DebugRenderer_primID;
        break;
      case GEOM_ID:
        sSh->renderSample = ispc::DebugRenderer_geomID;
        break;
      case INST_ID:
        sSh->renderSample = ispc::DebugRenderer_instID;
        break;
      case BACKFACING_NG:
        sSh->renderSample = ispc::DebugRenderer_backfacing_Ng;
        break;
      case BACKFACING_NS:
        sSh->renderSample = ispc::DebugRenderer_backfacing_Ns;
        break;
      case VOLUME:
        sSh->renderSample = ispc::DebugRenderer_volume;
        break;
      case TEST_FRAME:
      default:
        sSh->renderSample = ispc::DebugRenderer_testFrame;
        break;
      }
    });
  });
  event.wait();
}
*/
#endif

} // namespace ospray
