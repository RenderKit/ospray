// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OBJ_ispc.h"
#include "common/Material.h"
#include "texture/Texture2D.h"

namespace ospray {
namespace pathtracer {

struct OBJMaterial : public ospray::Material
{
  //! \brief common function to help printf-debugging
  /*! Every derived class should override this! */
  virtual std::string toString() const override
  {
    return "ospray::pathtracer::obj";
  }

  //! \brief commit the material's parameters
  virtual void commit() override
  {
    if (getIE() == nullptr)
      ispcEquivalent = ispc::PathTracer_OBJ_create();

    Texture2D *map_d = (Texture2D *)getParamObject("map_d");
    affine2f xform_d = getTextureTransform("map_d");
    Texture2D *map_Kd = (Texture2D *)getParamObject("map_kd");
    affine2f xform_Kd = getTextureTransform("map_kd");
    Texture2D *map_Ks = (Texture2D *)getParamObject("map_ks");
    affine2f xform_Ks = getTextureTransform("map_ks");
    Texture2D *map_Ns = (Texture2D *)getParamObject("map_ns");
    affine2f xform_Ns = getTextureTransform("map_ns");
    Texture2D *map_Bump = (Texture2D *)getParamObject("map_bump");
    affine2f xform_Bump = getTextureTransform("map_bump");
    linear2f rot_Bump = xform_Bump.l.orthogonal().transposed();

    const float d = getParam<float>("d", 1.f);
    vec3f Kd = getParam<vec3f>("kd", vec3f(0.8f));
    vec3f Ks = getParam<vec3f>("ks", vec3f(0.f));
    const float Ns = getParam<float>("ns", 10.f);
    vec3f Tf = getParam<vec3f>("tf", vec3f(0.0f));

    const float color_total = reduce_max(Kd + Ks + Tf);
    if (color_total > 1.0) {
      postStatusMsg(OSP_LOG_DEBUG) << "#osp:PT OBJ material produces energy "
                                   << "(kd + ks + tf = " << color_total
                                   << ", should be <= 1). Scaling down to 1.";
      Kd /= color_total;
      Ks /= color_total;
      Tf /= color_total;
    }

    ispc::PathTracer_OBJ_set(ispcEquivalent,
        map_d ? map_d->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_d,
        d,
        map_Kd ? map_Kd->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_Kd,
        (const ispc::vec3f &)Kd,
        map_Ks ? map_Ks->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_Ks,
        (const ispc::vec3f &)Ks,
        map_Ns ? map_Ns->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_Ns,
        Ns,
        (const ispc::vec3f &)Tf,
        map_Bump ? map_Bump->getIE() : nullptr,
        (const ispc::AffineSpace2f &)xform_Bump,
        (const ispc::LinearSpace2f &)rot_Bump);
  }
};

OSP_REGISTER_MATERIAL(pathtracer, OBJMaterial, default);
OSP_REGISTER_MATERIAL(pathtracer, OBJMaterial, obj);
} // namespace pathtracer
} // namespace ospray
