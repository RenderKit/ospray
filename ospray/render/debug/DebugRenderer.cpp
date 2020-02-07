// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "DebugRenderer.h"
#include "DebugRendererType.h"
// ispc exports
#include "DebugRenderer_ispc.h"

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

DebugRenderer::DebugRenderer()
{
  ispcEquivalent = ispc::DebugRenderer_create(this);
}

std::string DebugRenderer::toString() const
{
  return "ospray::DebugRenderer";
}

void DebugRenderer::commit()
{
  Renderer::commit();
  std::string method = getParam<std::string>("method", "eyeLight");
  ispc::DebugRenderer_set(getIE(), typeFromString(method));
}

OSP_REGISTER_RENDERER(DebugRenderer, debug);

} // namespace ospray
