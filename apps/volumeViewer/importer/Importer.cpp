// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// own
#include "Importer.h"
// ospcommon
#include "common/FileName.h"

namespace ospray {
  namespace vv_importer {

    void importOSP(const FileName &fileName, Group *existingGroupToAddTo);
    void importRM(const FileName &fileName, Group *existingGroupToAddTo);

    Group *import(const std::string &fn, Group *existingGroupToAddTo)
    {
      FileName fileName = fn;
      Group *group = existingGroupToAddTo;
      if (!group) group = new Group;

      if (fileName.ext() == "osp") {
        importOSP(fn, group);
      } else if (fileName.ext() == "bob") {
        importRM(fn, group);
      } else {
        throw std::runtime_error("#ospray:vv: do not know how to import file of type "+fileName.ext());
      }

      return group;
    }

  }
}
