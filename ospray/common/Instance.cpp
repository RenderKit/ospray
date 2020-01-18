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

// ospray
#include "Data.h"
#include "Instance.h"
// ispc exports
#include "Geometry_ispc.h"
#include "Instance_ispc.h"
#include "OSPCommon_ispc.h"

namespace ospray {

  extern "C" void *ospray_getEmbreeDevice();

  Instance::Instance(Group *_group)
  {
    managedObjectType    = OSP_INSTANCE;
    this->ispcEquivalent = ispc::Instance_create(this);

    group = _group;
  }

  std::string Instance::toString() const
  {
    return "ospray::Instance";
  }

  void Instance::commit()
  {
    instanceXfm = getParam<affine3f>("xfm", affine3f(one));
    rcpXfm      = rcp(instanceXfm);

    ispc::Instance_set(getIE(),
                       group->getIE(),
                       (ispc::AffineSpace3f &)instanceXfm,
                       (ispc::AffineSpace3f &)rcpXfm);
  }

  box3f Instance::getBounds() const
  {
    return xfmBounds(instanceXfm, group->getBounds());
  }

  OSPTYPEFOR_DEFINITION(Instance *);

}  // namespace ospray
