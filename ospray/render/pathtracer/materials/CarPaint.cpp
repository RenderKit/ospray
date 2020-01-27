// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CarPaint_ispc.h"
#include "common/Data.h"
#include "common/Material.h"
#include "math/spectrum.h"
#include "texture/Texture2D.h"

namespace ospray {
namespace pathtracer {

struct CarPaint : public ospray::Material
{
  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::CarPaint";
  }

  CarPaint()
  {
    ispcEquivalent = ispc::PathTracer_CarPaint_create();
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    MaterialParam3f baseColor = getMaterialParam3f("baseColor", vec3f(0.8f));
    MaterialParam1f roughness = getMaterialParam1f("roughness", 0.f);
    MaterialParam1f normal = getMaterialParam1f("normal", 1.f);

    MaterialParam1f flakeScale = getMaterialParam1f("flakeScale", 100.f);
    MaterialParam1f flakeDensity = getMaterialParam1f("flakeDensity", 0.f);
    MaterialParam1f flakeSpread = getMaterialParam1f("flakeSpread", 0.3f);
    MaterialParam1f flakeJitter = getMaterialParam1f("flakeJitter", 0.75f);
    MaterialParam1f flakeRoughness = getMaterialParam1f("flakeRoughness", 0.3f);

    MaterialParam1f coat = getMaterialParam1f("coat", 1.f);
    MaterialParam1f coatIor = getMaterialParam1f("coatIor", 1.5f);
    MaterialParam3f coatColor = getMaterialParam3f("coatColor", vec3f(1.f));
    MaterialParam1f coatThickness = getMaterialParam1f("coatThickness", 1.f);
    MaterialParam1f coatRoughness = getMaterialParam1f("coatRoughness", 0.f);
    MaterialParam1f coatNormal = getMaterialParam1f("coatNormal", 1.f);

    MaterialParam3f flipflopColor =
        getMaterialParam3f("flipflopColor", vec3f(1.f));
    MaterialParam1f flipflopFalloff =
        getMaterialParam1f("flipflopFalloff", 1.f);

    ispc::PathTracer_CarPaint_set(getIE(),
        (const ispc::vec3f &)baseColor.factor,
        baseColor.map ? baseColor.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)baseColor.xform,
        roughness.factor,
        roughness.map ? roughness.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)roughness.xform,
        normal.factor,
        normal.map ? normal.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)normal.xform,
        (const ispc::LinearSpace2f &)normal.rot,

        flakeScale.factor,
        flakeScale.map ? flakeScale.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)flakeScale.xform,
        flakeDensity.factor,
        flakeDensity.map ? flakeDensity.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)flakeDensity.xform,
        flakeSpread.factor,
        flakeSpread.map ? flakeSpread.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)flakeSpread.xform,
        flakeJitter.factor,
        flakeJitter.map ? flakeJitter.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)flakeJitter.xform,
        flakeRoughness.factor,
        flakeRoughness.map ? flakeRoughness.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)flakeRoughness.xform,

        coat.factor,
        coat.map ? coat.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)coat.xform,
        coatIor.factor,
        coatIor.map ? coatIor.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)coatIor.xform,
        (const ispc::vec3f &)coatColor.factor,
        coatColor.map ? coatColor.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)coatColor.xform,
        coatThickness.factor,
        coatThickness.map ? coatThickness.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)coatThickness.xform,
        coatRoughness.factor,
        coatRoughness.map ? coatRoughness.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)coatRoughness.xform,
        coatNormal.factor,
        coatNormal.map ? coatNormal.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)coatNormal.xform,
        (const ispc::LinearSpace2f &)coatNormal.rot,

        (const ispc::vec3f &)flipflopColor.factor,
        flipflopColor.map ? flipflopColor.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)flipflopColor.xform,
        flipflopFalloff.factor,
        flipflopFalloff.map ? flipflopFalloff.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)flipflopFalloff.xform);
  }
};

OSP_REGISTER_MATERIAL(pathtracer, CarPaint, carPaint);
} // namespace pathtracer
} // namespace ospray
