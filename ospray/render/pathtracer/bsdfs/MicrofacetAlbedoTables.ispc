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

uniform float* uniform MicrofacetDielectricAlbedoTable_values = NULL;
uniform float* uniform MicrofacetDielectricAlbedoTable_avgValues = NULL;

void MicrofacetDielectricAlbedoTable_precompute()
{
  if (MicrofacetDielectricAlbedoTable_values) return;

  uniform int size = MICROFACET_DIELECTRIC_ALBEDO_TABLE_SIZE;
  MicrofacetDielectricAlbedoTable_values = uniform new float[size*size*size];
  uniform float* uniform values = MicrofacetDielectricAlbedoTable_values;

  uniform int offset = 0;
  for (uniform int k = 0; k < size; k++)
  {
    for (uniform int j = 0; j < size; j++)
    {
      foreach (i = 0 ... size)
      {
        const float cosThetaO = (float)i / (size-1);
        const float eta = (float)j / (size-1);
        const float roughness = (float)k / (size-1);

        const float value = MicrofacetDielectricAlbedo_integrate(cosThetaO, eta, roughness);
        values[offset+i] = value;
      }
      offset += size;
    }
  }
}