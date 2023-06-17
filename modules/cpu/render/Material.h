// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDeviceObject.h"
#include "common/FeatureFlagsEnum.h"
#include "common/ObjectFactory.h"
#include "common/StructShared.h"
// ispc shared
#include "MaterialShared.h"
#include "bsdfs/MicrofacetAlbedoTables.h"
#include "texture/TextureParamShared.h"

namespace ospray {

/*! helper structure to store a uniform or textured material parameter */
template <typename T>
struct MaterialParam
{
  T factor;
  ispc::TextureParam tex;
  linear2f rot;
};

using MaterialParam1f = MaterialParam<float>;
using MaterialParam3f = MaterialParam<vec3f>;

struct OSPRAY_SDK_INTERFACE Material
    : public AddStructShared<ISPCDeviceObject, ispc::Material>,
      public ObjectFactory<Material, api::ISPCDevice &>
{
  Material(api::ISPCDevice &device, const FeatureFlagsOther ffo);
  virtual ~Material() override;
  virtual std::string toString() const override;
  virtual void commit() override;

  FeatureFlags getFeatureFlags() const;

  // helper function to get all texture related parameters
  ispc::TextureParam getTextureParam(const char *texture_name);

  /*! \brief helper function to get a uniform or texture material parameter */
  MaterialParam1f getMaterialParam1f(const char *name, float valIfNotFound);
  MaterialParam3f getMaterialParam3f(const char *name, vec3f valIfNotFound);

  bool isEmissive() const;

 private:
  /*! \brief helper function to combine multiple texture transformation
     parameters

     The following parameters (prefixed with "texture_name.") are combined
     into one transformation matrix:

     name         type   description
     transform    vec4f  interpreted as 2x2 matrix (linear part), column-major
     rotation     float  angle in degree, counterclock-wise, around center (0.5,
     0.5) scale        vec2f  enlarge texture, relative to center (0.5, 0.5)
     translation  vec2f  move texture in positive direction (right/up)

     The transformations are applied in the given order. Rotation, scale and
     translation are interpreted "texture centric", i.e. their effect seen by
     an user are relative to the texture (although the transformations are
     applied to the texture coordinates).
   */
  utility::Optional<affine2f> getTextureTransform2f(const char *_texname);
  utility::Optional<affine3f> getTextureTransform3f(const char *_texname);

  FeatureFlagsOther featureFlags;

  // Microfacet albedo tables data, shared by all materials. New materials
  // increment the use count, and decrement it on destruction to ensure
  // the data will be released before the device
  static Ref<MicrofacetAlbedoTables> microfacetAlbedoTables;
};

OSPTYPEFOR_SPECIALIZATION(Material *, OSP_MATERIAL);

// Inlined definitions /////////////////////////////////////////////////////////

inline FeatureFlags Material::getFeatureFlags() const
{
  FeatureFlags ff;
  ff.other = featureFlags;
  return ff;
}

inline bool Material::isEmissive() const
{
  return reduce_max(getSh()->emission) > 0.f;
}
} // namespace ospray
