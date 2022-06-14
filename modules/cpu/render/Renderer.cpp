// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Renderer.h"
#include "LoadBalancer.h"
#include "Material.h"
#include "common/Instance.h"
#include "common/Util.h"
#include "geometry/GeometricModel.h"
#include "pf/PixelFilter.h"
// ispc exports
#include "render/Renderer_ispc.h"
#include "render/util_ispc.h"

namespace ospray {

static FactoryMap<Renderer> g_renderersMap;

// Renderer definitions ///////////////////////////////////////////////////////

Renderer::Renderer()
{
  managedObjectType = OSP_RENDERER;
  pixelFilter = nullptr;
  getSh()->renderSample = ispc::Renderer_default_renderSample_addr();
}

std::string Renderer::toString() const
{
  return "ospray::Renderer";
}

void Renderer::commit()
{
  spp = std::max(1, getParam<int>("pixelSamples", 1));
  const int32 maxDepth = std::max(0, getParam<int>("maxPathLength", 20));
  const float minContribution = getParam<float>("minContribution", 0.001f);
  errorThreshold = getParam<float>("varianceThreshold", 0.f);

  maxDepthTexture = (Texture2D *)getParamObject("map_maxDepth");
  backplate = (Texture2D *)getParamObject("map_backplate");

  if (maxDepthTexture) {
    if (maxDepthTexture->format != OSP_TEXTURE_R32F
        || maxDepthTexture->filter != OSP_TEXTURE_FILTER_NEAREST) {
      static WarnOnce warning(
          "maxDepthTexture provided to the renderer "
          "needs to be of type OSP_TEXTURE_R32F and have "
          "the OSP_TEXTURE_FILTER_NEAREST flag");
    }
  }

  vec3f bgColor3 = getParam<vec3f>(
      "backgroundColor", vec3f(getParam<float>("backgroundColor", 0.f)));
  bgColor = getParam<vec4f>("backgroundColor", vec4f(bgColor3, 0.f));

  materialData = getParamDataT<Material *>("material");

  setupPixelFilter();

  if (materialData)
    ispcMaterialPtrs = createArrayOfSh<ispc::Material>(*materialData);
  else
    ispcMaterialPtrs.clear();

  getSh()->spp = spp;
  getSh()->maxDepth = maxDepth;
  getSh()->minContribution = minContribution;
  getSh()->bgColor = bgColor;
  getSh()->backplate = backplate ? backplate->getSh() : nullptr;
  getSh()->numMaterials = ispcMaterialPtrs.size();
  getSh()->material = ispcMaterialPtrs.data();
  getSh()->maxDepthTexture =
      maxDepthTexture ? maxDepthTexture->getSh() : nullptr;
  getSh()->pixelFilter =
      (ispc::PixelFilter *)(pixelFilter ? pixelFilter->getIE() : nullptr);

  ispc::precomputeZOrder();
}

Renderer *Renderer::createInstance(const char *type)
{
  return createInstanceHelper(type, g_renderersMap[type]);
}

void Renderer::registerType(const char *type, FactoryFcn<Renderer> f)
{
  g_renderersMap[type] = f;
}

void Renderer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  ispc::Renderer_renderTasks(getSh(),
      fb->getSh(),
      camera->getSh(),
      world->getSh(),
      perFrameData,
      taskIDs.data(),
      taskIDs.size());
}

OSPPickResult Renderer::pick(
    FrameBuffer *fb, Camera *camera, World *world, const vec2f &screenPos)
{
  OSPPickResult res;

  res.instance = nullptr;
  res.model = nullptr;
  res.primID = RTC_INVALID_GEOMETRY_ID;

  int instID = RTC_INVALID_GEOMETRY_ID;
  int geomID = RTC_INVALID_GEOMETRY_ID;
  int primID = RTC_INVALID_GEOMETRY_ID;

  ispc::Renderer_pick(getSh(),
      fb->getSh(),
      camera->getSh(),
      world->getSh(),
      (const ispc::vec2f &)screenPos,
      (ispc::vec3f &)res.worldPosition[0],
      instID,
      geomID,
      primID,
      res.hasHit);

  if (res.hasHit) {
    auto *instance = (*world->instances)[instID];
    auto *group = instance->group.ptr;
    if (!group->geometricModels) {
      res.hasHit = false;
      return res;
    }
    auto *model = (*group->geometricModels)[geomID];

    instance->refInc();
    model->refInc();

    res.instance = (OSPInstance)instance;
    res.model = (OSPGeometricModel)model;
    res.primID = static_cast<uint32_t>(primID);
  }

  return res;
}

void Renderer::setupPixelFilter()
{
  OSPPixelFilterTypes pixelFilterType =
      (OSPPixelFilterTypes)getParam<uint8_t>("pixelFilter",
          getParam<int32_t>(
              "pixelFilter", OSPPixelFilterTypes::OSP_PIXELFILTER_GAUSS));
  pixelFilter = nullptr;
  switch (pixelFilterType) {
  case OSPPixelFilterTypes::OSP_PIXELFILTER_BOX: {
    pixelFilter = rkcommon::make_unique<ospray::BoxPixelFilter>();
    break;
  }
  case OSPPixelFilterTypes::OSP_PIXELFILTER_BLACKMAN_HARRIS: {
    pixelFilter = rkcommon::make_unique<ospray::BlackmanHarrisLUTPixelFilter>();
    break;
  }
  case OSPPixelFilterTypes::OSP_PIXELFILTER_MITCHELL: {
    pixelFilter =
        rkcommon::make_unique<ospray::MitchellNetravaliLUTPixelFilter>();
    break;
  }
  case OSPPixelFilterTypes::OSP_PIXELFILTER_POINT: {
    pixelFilter = rkcommon::make_unique<ospray::PointPixelFilter>();
    break;
  }
  case OSPPixelFilterTypes::OSP_PIXELFILTER_GAUSS:
  default: {
    pixelFilter = rkcommon::make_unique<ospray::GaussianLUTPixelFilter>();
    break;
  }
  }
}

OSPTYPEFOR_DEFINITION(Renderer *);

} // namespace ospray
