// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MicrofacetAlbedo.ih"
#include "MicrofacetAlbedoTables.ih"
#include "MicrofacetAlbedoTablesShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

static inline uniform float MicrofacetAlbedoTable_integrateAvg(
    const uniform float *uniform f,
    uniform int size,
    uniform int numSamples = 1024)
{
  // Trapezoidal rule
  const uniform int n = numSamples + 1;
  float sum = 0.f;
#ifdef OSPRAY_TARGET_SYCL
  for (int i = 0; i < n; ++i) {
#else
  foreach (i = 0 ... n) {
#endif
    const float cosThetaO = (float)i / (n - 1);
    const float x = cosThetaO * (size - 1);
    sum += interp1DLinear(x, f, size) * cosThetaO
        * ((i == 0 || i == n - 1) ? 0.5f : 1.f);
  }

  uniform float totalSum = reduce_add(sum);
  return min(2.f * (totalSum / (n - 1)), 1.f);
}

static void MicrofacetAlbedoTable_precompute(
    MicrofacetAlbedoTables *uniform tables)
{
  const uniform int size = MICROFACET_ALBEDO_TABLE_SIZE;

  uniform float *uniform dirPtr = tables->albedo_dir;
  uniform float *uniform avgPtr = tables->albedo_avg;

  for (uniform int j = 0; j < size; j++) {
    const float roughness = (float)j / (size - 1);
    // compute the direction albedo for each cosThetaO
#ifdef OSPRAY_TARGET_SYCL
    for (int i = 0; i < size; ++i) {
#else
    foreach (i = 0 ... size) {
#endif
      const float cosThetaO = (float)i / (size - 1);
      dirPtr[i] = MicrofacetAlbedo_integrate(cosThetaO, roughness);
    }

    // compute the average albedo
    *avgPtr = MicrofacetAlbedoTable_integrateAvg(dirPtr, size);

    dirPtr += size;
    avgPtr++;
  }
}

typedef float (*MicrofacetDielectricAlbedo_IntegrateFunc)(
    float cosThetaO, float eta, float roughness, uniform int numSamples);

static void MicrofacetDielectricAlbedoTable_precompute(
    MicrofacetDielectricAlbedo_IntegrateFunc func,
    uniform int size,
    uniform float minEta,
    uniform float maxEta,
    uniform float *uniform dirValues,
    uniform float *uniform avgValues)
{
  const uniform int numSamples = 1024;

  uniform float *uniform dirPtr = dirValues;
  uniform float *uniform avgPtr = avgValues;

  for (uniform int k = 0; k < size; k++) {
    const float roughness = (float)k / (size - 1);
    for (uniform int j = 0; j < size; j++) {
      const float eta = lerp((float)j / (size - 1), minEta, maxEta);
      // compute the direction albedo for each cosThetaO
#ifdef OSPRAY_TARGET_SYCL
      for (int i = 0; i < size; ++i) {
#else
      foreach (i = 0 ... size) {
#endif
        const float cosThetaO = (float)i / (size - 1);
        dirPtr[i] = func(cosThetaO, eta, roughness, numSamples);
      }

      // compute the average albedo
      *avgPtr = MicrofacetAlbedoTable_integrateAvg(dirPtr, size);

      dirPtr += size;
      avgPtr++;
    }
  }
}

static void MicrofacetDielectricAlbedoTable_precompute(
    MicrofacetAlbedoTables *uniform tables)
{
  const uniform int size = MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE;
  const uniform float minEta = rcp(MICROFACET_DIELECTRIC_ALBEDO_TABLE_MAX_IOR);
  const uniform float maxEta = rcp(MICROFACET_DIELECTRIC_ALBEDO_TABLE_MIN_IOR);

  MicrofacetDielectricAlbedoTable_precompute(
      &MicrofacetDielectricAlbedo_integrate,
      size,
      minEta,
      maxEta,
      tables->dielectricAlbedo_dir,
      tables->dielectricAlbedo_avg);

  MicrofacetDielectricAlbedoTable_precompute(
      &MicrofacetDielectricReflectionAlbedo_integrate,
      size,
      minEta,
      maxEta,
      tables->dielectricReflectionAlbedo_dir,
      tables->dielectricReflectionAlbedo_avg);
}

static void MicrofacetDielectricRcpEtaAlbedoTable_precompute(
    MicrofacetAlbedoTables *uniform tables)
{
  const uniform int size = MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE;
  const uniform float minEta = MICROFACET_DIELECTRIC_ALBEDO_TABLE_MIN_IOR;
  const uniform float maxEta = MICROFACET_DIELECTRIC_ALBEDO_TABLE_MAX_IOR;

  MicrofacetDielectricAlbedoTable_precompute(
      &MicrofacetDielectricAlbedo_integrate,
      size,
      minEta,
      maxEta,
      tables->dielectricRcpEtaAlbedo_dir,
      tables->dielectricRcpEtaAlbedo_avg);
}

static void MicrofacetSheenAlbedoTable_precompute(
    MicrofacetAlbedoTables *uniform tables)
{
  const uniform int size = MICROFACET_SHEEN_ALBEDO_TABLE_SIZE;

  uniform float *uniform dirPtr = tables->sheenAlbedo_dir;

  for (uniform int j = 0; j < size; j++) {
    const float roughness = (float)j / (size - 1);
    // compute the direction albedo for each cosThetaO
#ifdef OSPRAY_TARGET_SYCL
    for (int i = 0; i < size; ++i) {
#else
    foreach (i = 0 ... size) {
#endif
      const float cosThetaO = (float)i / (size - 1);
      dirPtr[i] = MicrofacetSheenAlbedo_integrate(cosThetaO, roughness);
    }

    dirPtr += size;
  }
}

export void precomputeMicrofacetAlbedoTables(void *uniform _tables)
{
  MicrofacetAlbedoTables *uniform tables =
      (MicrofacetAlbedoTables * uniform) _tables;

  MicrofacetAlbedoTable_precompute(tables);
  MicrofacetDielectricAlbedoTable_precompute(tables);
  MicrofacetDielectricRcpEtaAlbedoTable_precompute(tables);
  MicrofacetSheenAlbedoTable_precompute(tables);
}

OSPRAY_END_ISPC_NAMESPACE
