// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MicrofacetAlbedo.ih"
#include "MicrofacetAlbedoTables.ih"

uniform bool MicrofacetAlbedoTable_initialized = false;
uniform float MicrofacetAlbedoTable_dir[MICROFACET_ALBEDO_TABLE_SIZE
    * MICROFACET_ALBEDO_TABLE_SIZE];
uniform float MicrofacetAlbedoTable_avg[MICROFACET_ALBEDO_TABLE_SIZE];
uniform float
    MicrofacetDielectricAlbedoTable_dir[MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
        * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
        * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE];
uniform float
    MicrofacetDielectricAlbedoTable_avg[MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
        * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE];
uniform float MicrofacetDielectricRcpEtaAlbedoTable_dir
    [MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
        * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
        * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE];
uniform float MicrofacetDielectricRcpEtaAlbedoTable_avg
    [MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
        * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE];
uniform float MicrofacetDielectricReflectionAlbedoTable_dir
    [MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
        * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
        * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE];
uniform float MicrofacetDielectricReflectionAlbedoTable_avg
    [MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE
        * MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE];
// uniform float* uniform MicrofacetDielectricReflectionRcpEtaAlbedoTable_dir =
// NULL; uniform float* uniform
// MicrofacetDielectricReflectionRcpEtaAlbedoTable_avg = NULL;
uniform float MicrofacetSheenAlbedoTable_dir[MICROFACET_SHEEN_ALBEDO_TABLE_SIZE
    * MICROFACET_SHEEN_ALBEDO_TABLE_SIZE];

static inline uniform float MicrofacetAlbedoTable_integrateAvg(
    const uniform float *uniform f,
    uniform int size,
    uniform int numSamples = 1024)
{
  // Trapezoidal rule
  const uniform int n = numSamples + 1;
  float sum = 0.f;
  foreach (i = 0 ... n) {
    const float cosThetaO = (float)i / (n - 1);
    const float x = cosThetaO * (size - 1);
    sum += interp1DLinear(x, f, size) * cosThetaO
        * ((i == 0 || i == n - 1) ? 0.5f : 1.f);
  }

  uniform float totalSum = reduce_add(sum);
  return min(2.f * (totalSum / (n - 1)), 1.f);
}

static void MicrofacetAlbedoTable_precompute()
{
  const uniform int size = MICROFACET_ALBEDO_TABLE_SIZE;

  uniform float *uniform dirPtr = MicrofacetAlbedoTable_dir;
  uniform float *uniform avgPtr = MicrofacetAlbedoTable_avg;

  for (uniform int j = 0; j < size; j++) {
    const float roughness = (float)j / (size - 1);
    // compute the direction albedo for each cosThetaO
    foreach (i = 0 ... size) {
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
      foreach (i = 0 ... size) {
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

static void MicrofacetDielectricAlbedoTable_precompute()
{
  const uniform int size = MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE;
  const uniform float minEta = rcp(MICROFACET_DIELECTRIC_ALBEDO_TABLE_MAX_IOR);
  const uniform float maxEta = rcp(MICROFACET_DIELECTRIC_ALBEDO_TABLE_MIN_IOR);

  MicrofacetDielectricAlbedoTable_precompute(
      &MicrofacetDielectricAlbedo_integrate,
      size,
      minEta,
      maxEta,
      MicrofacetDielectricAlbedoTable_dir,
      MicrofacetDielectricAlbedoTable_avg);

  MicrofacetDielectricAlbedoTable_precompute(
      &MicrofacetDielectricReflectionAlbedo_integrate,
      size,
      minEta,
      maxEta,
      MicrofacetDielectricReflectionAlbedoTable_dir,
      MicrofacetDielectricReflectionAlbedoTable_avg);
}

static void MicrofacetDielectricRcpEtaAlbedoTable_precompute()
{
  const uniform int size = MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE;
  const uniform float minEta = MICROFACET_DIELECTRIC_ALBEDO_TABLE_MIN_IOR;
  const uniform float maxEta = MICROFACET_DIELECTRIC_ALBEDO_TABLE_MAX_IOR;

  MicrofacetDielectricAlbedoTable_precompute(
      &MicrofacetDielectricAlbedo_integrate,
      size,
      minEta,
      maxEta,
      MicrofacetDielectricRcpEtaAlbedoTable_dir,
      MicrofacetDielectricRcpEtaAlbedoTable_avg);

  /*
  MicrofacetDielectricReflectionRcpEtaAlbedoTable_dir = uniform new
  float[size*size*size]; MicrofacetDielectricReflectionRcpEtaAlbedoTable_avg =
  uniform new float[size*size];
  MicrofacetDielectricAlbedoTable_precompute(&MicrofacetDielectricReflectionAlbedo_integrate,
                                             size, minEta, maxEta,
                                             MicrofacetDielectricReflectionRcpEtaAlbedoTable_dir,
                                             MicrofacetDielectricReflectionRcpEtaAlbedoTable_avg);
  */
}

static void MicrofacetSheenAlbedoTable_precompute()
{
  const uniform int size = MICROFACET_SHEEN_ALBEDO_TABLE_SIZE;

  uniform float *uniform dirPtr = MicrofacetSheenAlbedoTable_dir;

  for (uniform int j = 0; j < size; j++) {
    const float roughness = (float)j / (size - 1);
    // compute the direction albedo for each cosThetaO
    foreach (i = 0 ... size) {
      const float cosThetaO = (float)i / (size - 1);
      dirPtr[i] = MicrofacetSheenAlbedo_integrate(cosThetaO, roughness);
    }

    dirPtr += size;
  }
}

void precomputeMicrofacetAlbedoTables()
{
  if (MicrofacetAlbedoTable_initialized)
    return;

  MicrofacetAlbedoTable_precompute();
  MicrofacetDielectricAlbedoTable_precompute();
  MicrofacetDielectricRcpEtaAlbedoTable_precompute();
  MicrofacetSheenAlbedoTable_precompute();

  MicrofacetAlbedoTable_initialized = true;
}
