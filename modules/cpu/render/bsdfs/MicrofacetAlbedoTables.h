// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include "ISPCDevice.h"
#include "common/Managed.h"
#include "common/StructShared.h"
// ispc shared
#include "MicrofacetAlbedoTablesShared.h"

namespace ospray {

/* Directional and average albedo tables for microfacet BSDFs
 * [Kulla and Conty, 2017, "Revisiting Physically Based Shading at
 * Imageworks"]
 *
 * These are stored in USM to reduce the kernel size and avoid issues about
 * global statics/initialization in SYCL where these are populated by a kernel,
 * not compile-time defined values
 */
struct OSPRAY_SDK_INTERFACE MicrofacetAlbedoTables
    : public AddStructShared<ManagedObject, ispc::MicrofacetAlbedoTables>
{
  MicrofacetAlbedoTables(api::ISPCDevice &device);

  virtual std::string toString() const override;

 private:
  // Microfacet GGX albedo tables
  // directional 2D table (cosThetaO, roughness)
  std::unique_ptr<BufferShared<float>> albedo_dir;
  // average 1D table (roughness)
  std::unique_ptr<BufferShared<float>> albedo_avg;

  // Microfacet GGX dielectric albedo table. eta in [1/3, 1]
  // directional 3D table (cosThetaO, eta, roughness)
  std::unique_ptr<BufferShared<float>> dielectricAlbedo_dir;
  // average 2D table (eta, roughness)
  std::unique_ptr<BufferShared<float>> dielectricAlbedo_avg;
  // eta in [1, 3]
  // directional 3D table (cosThetaO, eta, roughness)
  std::unique_ptr<BufferShared<float>> dielectricRcpEtaAlbedo_dir;
  // average 2D table (eta, roughness)
  std::unique_ptr<BufferShared<float>> dielectricRcpEtaAlbedo_avg;

  // Microfacet GGX dielectric reflection-only albedo table. eta in [1/3, 1]
  // directional 3D table (cosThetaO, eta, roughness)
  std::unique_ptr<BufferShared<float>> dielectricReflectionAlbedo_dir;
  // average 2D table (eta, roughness)
  std::unique_ptr<BufferShared<float>> dielectricReflectionAlbedo_avg;

  // Microfacet sheen albedo table
  // directional 2D table (cosThetaO, roughness)
  std::unique_ptr<BufferShared<float>> sheenAlbedo_dir;
};

} // namespace ospray
