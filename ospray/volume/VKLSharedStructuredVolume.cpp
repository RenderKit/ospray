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

#include "VKLSharedStructuredVolume.h"
#include "common/Data.h"

namespace ospray {

  std::string VKLSharedStructuredVolume::toString() const
  {
    return "ospray::volume::VKLSharedStructuredVolume";
  }

  void VKLSharedStructuredVolume::commit()
  {
    ispcEquivalent = ispc::Volume_createInstance_vklVolume(this);
    vklVolume = vklNewVolume("structured_regular");

    // pass all supported parameters through to VKL volume object
    std::for_each(params_begin(), params_end(), [&](std::shared_ptr<Param> &p) {
      auto &param = *p;

      if (param.data.is<vec3f>()) {
        vklSetVec3f(vklVolume,
                 param.name.c_str(),
                 param.data.get<vec3f>().x,
                 param.data.get<vec3f>().y,
                 param.data.get<vec3f>().z);
      } else if (param.data.is<vec3i>()) {
        vklSetVec3i(vklVolume,
                 param.name.c_str(),
                 param.data.get<vec3i>().x,
                 param.data.get<vec3i>().y,
                 param.data.get<vec3i>().z);
      } else if (param.data.is<ManagedObject *>()) {
        Data *data = (Data *)param.data.get<ManagedObject *>();
        if (data->type == OSP_FLOAT) {
          VKLData vklData = vklNewData(
              data->numItems.product(), VKL_FLOAT, data->data(), VKL_DATA_SHARED_BUFFER);
          vklSetData(vklVolume, param.name.c_str(), vklData);
          vklRelease(vklData);
        } else {
          std::cerr << "  only OSP_FLOAT data supported" << std::endl;
        }
      } else if (param.name == "gridType") {
        // don't pass through / silently ignore
      } else {
        std::cerr << "[VKLSharedStructuredVolume] ignoring unsupported "
                     "parameter type: "
                  << param.name << std::endl;
      }
    });
    vklCommit(vklVolume);
    (vkl_box3f&) bounds = vklGetBoundingBox(vklVolume);
    Volume::commit();
    ispc::Volume_set_vklVolume(ispcEquivalent, vklVolume, (ispc::box3f*)&bounds);
  }

  OSP_REGISTER_VOLUME(VKLSharedStructuredVolume, vkl_structured_volume);

}  // namespace ospray
