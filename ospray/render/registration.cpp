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
  Material::registerType<Alloy>("pathtracer", "alloy");
  Material::registerType<CarPaint>("pathtracer", "carPaint");
  Material::registerType<Glass>("pathtracer", "glass");
  Material::registerType<Luminous>("pathtracer", "luminous");
  Material::registerType<Metal>("pathtracer", "metal");
  Material::registerType<MetallicPaint>("pathtracer", "metallicPaint");
  Material::registerType<MixMaterial>("pathtracer", "mix");
  Material::registerType<OBJMaterial>("pathtracer", "obj");
  Material::registerType<Plastic>("pathtracer", "plastic");
  Material::registerType<Principled>("pathtracer", "principled");
  Material::registerType<ThinGlass>("pathtracer", "thinGlass");
  Material::registerType<Velvet>("pathtracer", "velvet");
}

} // namespace ospray
