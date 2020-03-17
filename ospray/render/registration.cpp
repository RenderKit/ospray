// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "debug/DebugRenderer.h"
#include "pathtracer/PathTracer.h"
#include "scivis/SciVis.h"

#include "pathtracer/materials/Alloy.h"
#include "pathtracer/materials/CarPaint.h"
#include "pathtracer/materials/Glass.h"
#include "pathtracer/materials/Luminous.h"
#include "pathtracer/materials/Metal.h"
#include "pathtracer/materials/MetallicPaint.h"
#include "pathtracer/materials/Mix.h"
#include "pathtracer/materials/OBJ.h"
#include "pathtracer/materials/Plastic.h"
#include "pathtracer/materials/Principled.h"
#include "pathtracer/materials/ThinGlass.h"
#include "pathtracer/materials/Velvet.h"
using namespace ospray::pathtracer;

#include "scivis/SciVisMaterial.h"

namespace ospray {

void registerAllRenderers()
{
  Renderer::registerType<DebugRenderer>("debug");
  Renderer::registerType<PathTracer>("pathtracer");
  Renderer::registerType<SciVis>("scivis");
  Renderer::registerType<SciVis>("ao");
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

  Material::registerType<SciVisMaterial>("scivis", "obj");
  Material::registerType<SciVisMaterial>("ao", "obj");
}

} // namespace ospray
