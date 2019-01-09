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

#undef NDEBUG

// ospray
#include "Isosurfaces.h"
#include "common/Data.h"
#include "common/Model.h"
// ispc-generated files
#include "Isosurfaces_ispc.h"

namespace ospray {

  Isosurfaces::Isosurfaces()
  {
    this->ispcEquivalent = ispc::Isosurfaces_create(this);
  }

  std::string Isosurfaces::toString() const
  {
    return "ospray::Isosurfaces";
  }

  void Isosurfaces::finalize(Model *model)
  {
    isovaluesData = getParamData("isovalues", nullptr);
    volume        = (Volume *)getParamObject("volume", nullptr);

    Assert(isovaluesData);
    Assert(isovaluesData->numItems > 0);
    Assert(volume);

    numIsovalues = isovaluesData->numItems;
    isovalues    = (float*)isovaluesData->data;

    ispc::Isosurfaces_set(getIE(), model->getIE(), numIsovalues,
                          isovalues, volume->getIE());
  }

  OSP_REGISTER_GEOMETRY(Isosurfaces, isosurfaces);

} // ::ospray
