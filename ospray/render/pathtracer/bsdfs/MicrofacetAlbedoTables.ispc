// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use self file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "MicrofacetAlbedo.ih"
#include "MicrofacetAlbedoTables.ih"

uniform float* uniform MicrofacetDielectricReflectionAlbedoTable_directional = NULL;
uniform float* uniform MicrofacetDielectricReflectionAlbedoTable_average = NULL;
uniform float* uniform MicrofacetDielectricReflectionRcpEtaAlbedoTable_directional = NULL;
uniform float* uniform MicrofacetDielectricReflectionRcpEtaAlbedoTable_average = NULL;

static inline uniform float MicrofacetAlbedoTable_integrateAverage(const uniform float* uniform f, uniform int size)
{
  float sum = 0.f;
  foreach (i = 0 ... size)
  {
    const float cosThetaO = (float)i / (size-1);
    sum += f[i] * cosThetaO;
  }
  uniform float totalSum = reduce_add(sum);
  return min(2.f * (totalSum / size), 1.f);
}

static void MicrofacetDielectricReflectionAlbedoTable_precompute()
{
  if (MicrofacetDielectricReflectionAlbedoTable_directional)
    return;

  const uniform int size = MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE;
  const uniform float minEta = rcp(MICROFACET_DIELECTRIC_ALBEDO_TABLE_MAX_IOR);
  const uniform float maxEta = rcp(MICROFACET_DIELECTRIC_ALBEDO_TABLE_MIN_IOR);

  uniform float* uniform directionalValues =
    MicrofacetDielectricReflectionAlbedoTable_directional = uniform new float[size*size*size];
  uniform float* uniform averageValues =
    MicrofacetDielectricReflectionAlbedoTable_average = uniform new float[size*size];

  for (uniform int k = 0; k < size; k++)
  {
    for (uniform int j = 0; j < size; j++)
    {
      // compute the direction albedo for each cosThetaO
      foreach (i = 0 ... size)
      {
        const float cosThetaO = (float)i / (size-1);
        const float eta = lerp((float)j / (size-1), minEta, maxEta);
        const float roughness = (float)k / (size-1);

        directionalValues[i] = MicrofacetDielectricReflectionAlbedo_integrate(cosThetaO, eta, roughness);
      }

      // compute the average albedo
      *averageValues = MicrofacetAlbedoTable_integrateAverage(directionalValues, size);

      directionalValues += size;
      averageValues++;
    }
  }
}

static void MicrofacetDielectricReflectionRcpEtaAlbedoTable_precompute()
{
  if (MicrofacetDielectricReflectionRcpEtaAlbedoTable_directional)
    return;

  const uniform int size = MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE;
  const uniform float minEta = MICROFACET_DIELECTRIC_ALBEDO_TABLE_MIN_IOR;
  const uniform float maxEta = MICROFACET_DIELECTRIC_ALBEDO_TABLE_MAX_IOR;

  uniform float* uniform directionalValues =
    MicrofacetDielectricReflectionRcpEtaAlbedoTable_directional = uniform new float[size*size*size];
  uniform float* uniform averageValues =
    MicrofacetDielectricReflectionRcpEtaAlbedoTable_average = uniform new float[size*size];

  for (uniform int k = 0; k < size; k++)
  {
    for (uniform int j = 0; j < size; j++)
    {
      // compute the direction albedo for each cosThetaO
      foreach (i = 0 ... size)
      {
        const float cosThetaO = (float)i / (size-1);
        const float eta = lerp((float)j / (size-1), minEta, maxEta);
        const float roughness = (float)k / (size-1);

        directionalValues[i] = MicrofacetDielectricReflectionAlbedo_integrate(cosThetaO, eta, roughness);
      }

      // compute the average albedo
      *averageValues = MicrofacetAlbedoTable_integrateAverage(directionalValues, size);

      directionalValues += size;
      averageValues++;
    }
  }
}

void precomputeMicrofacetAlbedoTables()
{
  MicrofacetDielectricReflectionAlbedoTable_precompute();
  MicrofacetDielectricReflectionRcpEtaAlbedoTable_precompute();
}