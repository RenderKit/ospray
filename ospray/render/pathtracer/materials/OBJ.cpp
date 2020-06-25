// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OBJ.h"
// ispc
#include "render/pathtracer/materials/OBJ_ispc.h"

namespace ospray {
namespace pathtracer {

OBJMaterial::OBJMaterial()
{
  ispcEquivalent = ispc::PathTracer_OBJ_create();
}

std::string OBJMaterial::toString() const
{
  return "ospray::pathtracer::obj";
}

void OBJMaterial::commit()
{
  MaterialParam1f d = getMaterialParam1f("d", 1.f);
  MaterialParam3f Kd = getMaterialParam3f("kd", vec3f(.8f));
  MaterialParam3f Ks = getMaterialParam3f("ks", vec3f(0.f));
  MaterialParam1f Ns = getMaterialParam1f("ns", 10.f);

  vec3f Tf = getParam<vec3f>("tf", vec3f(0.f));
  ispc::TextureParam bumpTex = getTextureParam("map_bump");
  linear2f bumpRot =
      ((linear2f *)(&bumpTex.xform2f.l))->orthogonal().transposed();

  const float color_total = reduce_max(Kd.factor + Ks.factor + Tf);
  if (color_total > 1.0) {
    postStatusMsg(OSP_LOG_DEBUG) << "#osp:PT OBJ material produces energy "
                                 << "(kd + ks + tf = " << color_total
                                 << ", should be <= 1). Scaling down to 1.";
    Kd.factor /= color_total;
    Ks.factor /= color_total;
    Tf /= color_total;
  }

  ispc::PathTracer_OBJ_set(ispcEquivalent,
      d.tex,
      d.factor,
      Kd.tex,
      (const ispc::vec3f &)Kd.factor,
      Ks.tex,
      (const ispc::vec3f &)Ks.factor,
      Ns.tex,
      Ns.factor,
      (const ispc::vec3f &)Tf,
      bumpTex,
      (const ispc::LinearSpace2f &)bumpRot);
}

} // namespace pathtracer
} // namespace ospray
