// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Principled_ispc.h"
#include "common/Data.h"
#include "common/Material.h"
#include "math/spectrum.h"
#include "texture/Texture2D.h"

namespace ospray {
namespace pathtracer {

struct Principled : public ospray::Material
{
  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::Principled";
  }

  Principled()
  {
    ispcEquivalent = ispc::PathTracer_Principled_create();
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    MaterialParam3f baseColor = getMaterialParam3f("baseColor", vec3f(0.8f));
    MaterialParam3f edgeColor = getMaterialParam3f("edgeColor", vec3f(1.f));
    MaterialParam1f metallic = getMaterialParam1f("metallic", 0.f);
    MaterialParam1f diffuse = getMaterialParam1f("diffuse", 1.f);
    MaterialParam1f specular = getMaterialParam1f("specular", 1.f);
    MaterialParam1f ior = getMaterialParam1f("ior", 1.f);
    MaterialParam1f transmission = getMaterialParam1f("transmission", 0.f);
    MaterialParam3f transmissionColor =
        getMaterialParam3f("transmissionColor", vec3f(1.f));
    MaterialParam1f transmissionDepth =
        getMaterialParam1f("transmissionDepth", 1.f);
    MaterialParam1f roughness = getMaterialParam1f("roughness", 0.f);
    MaterialParam1f anisotropy = getMaterialParam1f("anisotropy", 0.f);
    MaterialParam1f rotation = getMaterialParam1f("rotation", 0.f);
    MaterialParam1f normal = getMaterialParam1f("normal", 1.f);
    MaterialParam1f baseNormal = getMaterialParam1f("baseNormal", 1.f);

    MaterialParam1f coat = getMaterialParam1f("coat", 0.f);
    MaterialParam1f coatIor = getMaterialParam1f("coatIor", 1.5f);
    MaterialParam3f coatColor = getMaterialParam3f("coatColor", vec3f(1.f));
    MaterialParam1f coatThickness = getMaterialParam1f("coatThickness", 1.f);
    MaterialParam1f coatRoughness = getMaterialParam1f("coatRoughness", 0.f);
    MaterialParam1f coatNormal = getMaterialParam1f("coatNormal", 1.f);

    MaterialParam1f sheen = getMaterialParam1f("sheen", 0.f);
    MaterialParam3f sheenColor = getMaterialParam3f("sheenColor", vec3f(1.f));
    MaterialParam1f sheenTint = getMaterialParam1f("sheenTint", 0.f);
    MaterialParam1f sheenRoughness = getMaterialParam1f("sheenRoughness", 0.2f);

    MaterialParam1f opacity = getMaterialParam1f("opacity", 1.f);

    bool thin = getParam<bool>("thin", false);
    MaterialParam1f backlight = getMaterialParam1f("backlight", 0.f);
    MaterialParam1f thickness = getMaterialParam1f("thickness", 1.f);

    float outsideIor = getParam<float>("outsideIor", 1.f);
    vec3f outsideTransmissionColor =
        getParam<vec3f>("outsideTransmissionColor", vec3f(1.f));
    float outsideTransmissionDepth =
        getParam<float>("outsideTransmissionDepth", 1.f);

    ispc::PathTracer_Principled_set(getIE(),
        (const ispc::vec3f &)baseColor.factor,
        baseColor.map ? baseColor.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)baseColor.xform,
        (const ispc::vec3f &)edgeColor.factor,
        edgeColor.map ? edgeColor.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)edgeColor.xform,
        metallic.factor,
        metallic.map ? metallic.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)metallic.xform,
        diffuse.factor,
        diffuse.map ? diffuse.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)diffuse.xform,
        specular.factor,
        specular.map ? specular.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)specular.xform,
        ior.factor,
        ior.map ? ior.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)ior.xform,
        transmission.factor,
        transmission.map ? transmission.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)transmission.xform,
        (const ispc::vec3f &)transmissionColor.factor,
        transmissionColor.map ? transmissionColor.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)transmissionColor.xform,
        transmissionDepth.factor,
        transmissionDepth.map ? transmissionDepth.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)transmissionDepth.xform,
        roughness.factor,
        roughness.map ? roughness.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)roughness.xform,
        anisotropy.factor,
        anisotropy.map ? anisotropy.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)anisotropy.xform,
        rotation.factor,
        rotation.map ? rotation.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)rotation.xform,
        normal.factor,
        normal.map ? normal.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)normal.xform,
        (const ispc::LinearSpace2f &)normal.rot,
        baseNormal.factor,
        baseNormal.map ? baseNormal.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)baseNormal.xform,
        (const ispc::LinearSpace2f &)baseNormal.rot,

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

        sheen.factor,
        sheen.map ? sheen.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)sheen.xform,
        (const ispc::vec3f &)sheenColor.factor,
        sheenColor.map ? sheenColor.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)sheenColor.xform,
        sheenTint.factor,
        sheenTint.map ? sheenTint.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)sheenTint.xform,
        sheenRoughness.factor,
        sheenRoughness.map ? sheenRoughness.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)sheenRoughness.xform,

        opacity.factor,
        opacity.map ? opacity.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)opacity.xform,

        thin,
        backlight.factor,
        backlight.map ? backlight.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)backlight.xform,
        thickness.factor,
        thickness.map ? thickness.map->getIE() : nullptr,
        (const ispc::AffineSpace2f &)thickness.xform,

        outsideIor,
        (const ispc::vec3f &)outsideTransmissionColor,
        outsideTransmissionDepth);
  }
};

OSP_REGISTER_MATERIAL(pathtracer, Principled, principled);
} // namespace pathtracer
} // namespace ospray
