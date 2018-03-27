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

uniform float* uniform MicrofacetAlbedoTable_dir = NULL;
uniform float* uniform MicrofacetAlbedoTable_avg = NULL;
uniform float* uniform MicrofacetDielectricReflectionAlbedoTable_dir = NULL;
uniform float* uniform MicrofacetDielectricReflectionAlbedoTable_avg = NULL;
uniform float* uniform MicrofacetDielectricReflectionRcpEtaAlbedoTable_dir = NULL;
uniform float* uniform MicrofacetDielectricReflectionRcpEtaAlbedoTable_avg = NULL;

static inline uniform float MicrofacetAlbedoTable_integrateAvg(const uniform float* uniform f, uniform int size)
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

static void MicrofacetAlbedoTable_precompute()
{
  const uniform int size = MICROFACET_ALBEDO_TABLE_SIZE;

  uniform float* uniform dirPtr = MicrofacetAlbedoTable_dir = uniform new float[size*size];
  uniform float* uniform avgPtr = MicrofacetAlbedoTable_avg = uniform new float[size];

  for (uniform int j = 0; j < size; j++)
  {
    // compute the direction albedo for each cosThetaO
    foreach (i = 0 ... size)
    {
      const float cosThetaO = (float)i / (size-1);
      const float roughness = (float)j / (size-1);

      dirPtr[i] = MicrofacetAlbedo_integrate(cosThetaO, roughness);
    }

    // compute the average albedo
    *avgPtr = MicrofacetAlbedoTable_integrateAvg(dirPtr, size);

    dirPtr += size;
    avgPtr++;
  }
}

static void MicrofacetDielectricReflectionAlbedoTable_precompute(uniform int size,
                                                                 uniform float minEta, uniform float maxEta,
                                                                 uniform float* uniform dirValues,
                                                                 uniform float* uniform avgValues)
{
  uniform float* uniform dirPtr = dirValues;
  uniform float* uniform avgPtr = avgValues;

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

        dirPtr[i] = MicrofacetDielectricReflectionAlbedo_integrate(cosThetaO, eta, roughness);
      }

      // compute the average albedo
      *avgPtr = MicrofacetAlbedoTable_integrateAvg(dirPtr, size);

      dirPtr += size;
      avgPtr++;
    }
  }
}

static void MicrofacetDielectricReflectionAlbedoTable_precompute()
{
  const uniform int size = MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE;
  const uniform float minEta = rcp(MICROFACET_DIELECTRIC_ALBEDO_TABLE_MAX_IOR);
  const uniform float maxEta = rcp(MICROFACET_DIELECTRIC_ALBEDO_TABLE_MIN_IOR);

  MicrofacetDielectricReflectionAlbedoTable_dir = uniform new float[size*size*size];
  MicrofacetDielectricReflectionAlbedoTable_avg = uniform new float[size*size];

  MicrofacetDielectricReflectionAlbedoTable_precompute(size, minEta, maxEta,
                                                       MicrofacetDielectricReflectionAlbedoTable_dir,
                                                       MicrofacetDielectricReflectionAlbedoTable_avg);
}

static void MicrofacetDielectricReflectionRcpEtaAlbedoTable_precompute()
{
  const uniform int size = MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE;
  const uniform float minEta = MICROFACET_DIELECTRIC_ALBEDO_TABLE_MIN_IOR;
  const uniform float maxEta = MICROFACET_DIELECTRIC_ALBEDO_TABLE_MAX_IOR;

  MicrofacetDielectricReflectionRcpEtaAlbedoTable_dir = uniform new float[size*size*size];
  MicrofacetDielectricReflectionRcpEtaAlbedoTable_avg = uniform new float[size*size];

  MicrofacetDielectricReflectionAlbedoTable_precompute(size, minEta, maxEta,
                                                       MicrofacetDielectricReflectionRcpEtaAlbedoTable_dir,
                                                       MicrofacetDielectricReflectionRcpEtaAlbedoTable_avg);
}

void precomputeMicrofacetAlbedoTables()
{
  if (MicrofacetAlbedoTable_dir)
    return;

  MicrofacetAlbedoTable_precompute();
  MicrofacetDielectricReflectionAlbedoTable_precompute();
  MicrofacetDielectricReflectionRcpEtaAlbedoTable_precompute();
}