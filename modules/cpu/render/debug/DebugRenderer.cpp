// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "DebugRenderer.h"
#include "DebugRendererType.h"
// ispc exports
#include "render/debug/DebugRenderer_ispc.h"

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

DebugRenderer::DebugRenderer()
{
  getSh()->renderSample = ispc::DebugRenderer_testFrame_addr();
}

std::string DebugRenderer::toString() const
{
  return "ospray::DebugRenderer";
}

void DebugRenderer::commit()
{
  Renderer::commit();

  std::string method = getParam<std::string>("method", "eyeLight");
  switch (typeFromString(method)) {
  case RAY_DIR:
    getSh()->renderSample = ispc::DebugRenderer_rayDir_addr();
    break;
  case EYE_LIGHT:
    getSh()->renderSample = ispc::DebugRenderer_eyeLight_addr();
    break;
  case NG:
    getSh()->renderSample = ispc::DebugRenderer_Ng_addr();
    break;
  case NS:
    getSh()->renderSample = ispc::DebugRenderer_Ns_addr();
    break;
  case COLOR:
    getSh()->renderSample = ispc::DebugRenderer_vertexColor_addr();
    break;
  case TEX_COORD:
    getSh()->renderSample = ispc::DebugRenderer_texCoord_addr();
    break;
  case DPDS:
    getSh()->renderSample = ispc::DebugRenderer_dPds_addr();
    break;
  case DPDT:
    getSh()->renderSample = ispc::DebugRenderer_dPdt_addr();
    break;
  case PRIM_ID:
    getSh()->renderSample = ispc::DebugRenderer_primID_addr();
    break;
  case GEOM_ID:
    getSh()->renderSample = ispc::DebugRenderer_geomID_addr();
    break;
  case INST_ID:
    getSh()->renderSample = ispc::DebugRenderer_instID_addr();
    break;
  case BACKFACING_NG:
    getSh()->renderSample = ispc::DebugRenderer_backfacing_Ng_addr();
    break;
  case BACKFACING_NS:
    getSh()->renderSample = ispc::DebugRenderer_backfacing_Ns_addr();
    break;
  case VOLUME:
    getSh()->renderSample = ispc::DebugRenderer_volume_addr();
    break;
  case TEST_FRAME:
  default:
    getSh()->renderSample = ispc::DebugRenderer_testFrame_addr();
    break;
  }
}

} // namespace ospray
