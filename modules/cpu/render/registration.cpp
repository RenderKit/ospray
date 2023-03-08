// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ao/AORenderer.h"
#include "debug/DebugRenderer.h"
#include "pathtracer/PathTracer.h"
#include "scivis/SciVis.h"

#include "materials/Alloy.h"
#include "materials/CarPaint.h"
#include "materials/Glass.h"
#include "materials/Luminous.h"
#include "materials/Metal.h"
#include "materials/MetallicPaint.h"
#include "materials/Mix.h"
#include "materials/OBJ.h"
#include "materials/Plastic.h"
#include "materials/Principled.h"
#include "materials/ThinGlass.h"
#include "materials/Velvet.h"
using namespace ospray::pathtracer;

namespace ospray {

void registerAllRenderers()
{
  Renderer::registerType<DebugRenderer>("debug");
  Renderer::registerType<PathTracer>("pathtracer");
  Renderer::registerType<SciVis>("scivis");
  Renderer::registerType<AORenderer>("ao");
}

void registerAllMaterials()
{
  Material::registerType<Alloy>("alloy");
  Material::registerType<CarPaint>("carPaint");
  Material::registerType<Glass>("glass");
  Material::registerType<Luminous>("luminous");
  Material::registerType<Metal>("metal");
  Material::registerType<MetallicPaint>("metallicPaint");
#ifndef OSPRAY_TARGET_SYCL
  // Mix material isn't going to work on the GPU b/c we can't have
  // recursive calls and it may nest MultiBSDFs. Needs some special treatment
  Material::registerType<MixMaterial>("mix");
#endif
  Material::registerType<OBJMaterial>("obj");
  Material::registerType<Plastic>("plastic");
  Material::registerType<Principled>("principled");
  Material::registerType<ThinGlass>("thinGlass");
  Material::registerType<Velvet>("velvet");
}

} // namespace ospray
