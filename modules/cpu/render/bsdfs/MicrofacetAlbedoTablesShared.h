// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#define MICROFACET_ALBEDO_TABLE_SIZE 32

#define MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE 16
#define MICROFACET_DIELECTRIC_ALBEDO_TABLE_MIN_IOR 1.f
#define MICROFACET_DIELECTRIC_ALBEDO_TABLE_MAX_IOR 3.f

#define MICROFACET_SHEEN_ALBEDO_TABLE_SIZE 16

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct MicrofacetAlbedoTables
{
  // Microfacet GGX albedo tables
  // directional 2D table (cosThetaO, roughness)
  float *albedo_dir;
  // average 1D table (roughness)
  float *albedo_avg;

  // Microfacet GGX dielectric albedo table. eta in [1/3, 1]
  // directional 3D table (cosThetaO, eta, roughness)
  float *dielectricAlbedo_dir;
  // average 2D table (eta, roughness)
  float *dielectricAlbedo_avg;
  // eta in [1, 3]
  // directional 3D table (cosThetaO, eta, roughness)
  float *dielectricRcpEtaAlbedo_dir;
  // average 2D table (eta, roughness)
  float *dielectricRcpEtaAlbedo_avg;

  // Microfacet GGX dielectric reflection-only albedo table. eta in [1/3, 1]
  // directional 3D table (cosThetaO, eta, roughness)
  float *dielectricReflectionAlbedo_dir;
  // average 2D table (eta, roughness)
  float *dielectricReflectionAlbedo_avg;

  // Microfacet sheen albedo table
  // directional 2D table (cosThetaO, roughness)
  float *sheenAlbedo_dir;

#ifdef __cplusplus
  MicrofacetAlbedoTables()
      : albedo_dir(nullptr),
        albedo_avg(nullptr),
        dielectricAlbedo_dir(nullptr),
        dielectricAlbedo_avg(nullptr),
        dielectricRcpEtaAlbedo_dir(nullptr),
        dielectricRcpEtaAlbedo_avg(nullptr),
        dielectricReflectionAlbedo_dir(nullptr),
        dielectricReflectionAlbedo_avg(nullptr),
        sheenAlbedo_dir(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
