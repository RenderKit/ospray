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

#include "Managed.h"
#include "Data.h"
#include "OSPCommon_ispc.h"

namespace ospray {

  ManagedObject::~ManagedObject()
  {
    ispc::delete_uniform(ispcEquivalent);
    ispcEquivalent = nullptr;

    std::for_each(params_begin(), params_end(), [&](std::shared_ptr<Param> &p) {
      auto &param = *p;
      if (param.data.is<OSP_PTR>()) {
        auto *obj = param.data.get<OSP_PTR>();
        if (obj != nullptr)
          obj->refDec();
      }
    });
  }

  void ManagedObject::commit() {}

  std::string ManagedObject::toString() const
  {
    return "ospray::ManagedObject";
  }

  void ManagedObject::checkUnused()
  {
    for (auto p = params_begin(); p != params_end(); ++p) {
      if (!(*p)->query)
        postStatusMsg(1) << toString()
                         << ": found unused (or of wrong data type) parameter '"
                         << (*p)->name << "'";
    }
  }

  box3f ManagedObject::getBounds() const
  {
    return box3f(empty);
  }

  ManagedObject *ManagedObject::getParamObject(const char *name,
                                               ManagedObject *valIfNotFound)
  {
    return getParam<ManagedObject *>(name, valIfNotFound);
  }

  OSPTYPEFOR_DEFINITION(ManagedObject *);

}  // namespace ospray
