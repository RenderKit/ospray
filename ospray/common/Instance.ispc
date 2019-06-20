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

#include "Instance.ih"

export void *uniform Instance_create(void *uniform cppE)
{
  Instance *uniform inst = uniform new Instance;

  inst->cppEquivalent = cppE;

  inst->geometricModels    = NULL;
  inst->numGeometricModels = 0;

  inst->volumetricModels    = NULL;
  inst->numVolumetricModels = 0;

  return (void *uniform)inst;
}

export void Instance_set(void *uniform _inst,
                         void *uniform _geomModels,
                         uniform int32 numGeomModels,
                         void *uniform _volModels,
                         uniform int32 numVolModels,
                         const uniform AffineSpace3f &xfm,
                         const uniform AffineSpace3f &rcp_xfm)
{
  Instance *uniform inst = (Instance * uniform) _inst;

  inst->geometricModels    = (GeometricModel * *uniform) _geomModels;
  inst->numGeometricModels = numGeomModels;

  inst->volumetricModels    = (VolumetricModel * *uniform) _volModels;
  inst->numVolumetricModels = numVolModels;

  inst->xfm     = xfm;
  inst->rcp_xfm = rcp_xfm;
}
