// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
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

#include "common/Model.ih"
#include "embree3/rtcore.isph"

typedef uniform Geometry *uniform uniGeomPtr;
typedef uniform Material *uniform uniMaterialPtr;
typedef uniform Volume   *uniform uniVolumePtr;

export void *uniform DistributedModel_create(void *uniform cppE)
{
  uniform Model *uniform model = uniform new uniform Model;
  model->cppEquivalent     = cppE;
  model->embreeSceneHandle = NULL;
  model->geometry          = NULL;
  model->volumes           = NULL;
  return (void *uniform)model;
}

export void DistributedModel_setBounds(void *uniform _model,
                                       uniform box3f *uniform bounds)
{
  uniform Model *uniform model = (uniform Model *uniform)_model;
  model->bounds = *bounds;
}

