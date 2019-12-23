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
#include "volume/Volume.h"
#include "Volume_ispc.h"
#include "common/Data.h"
#include "common/Util.h"
#include "transferFunction/TransferFunction.h"

#include "openvkl/openvkl.h"

#include <unordered_map>

namespace ospray {

  // Volume defintions ////////////////////////////////////////////////////////

  Volume::Volume(const std::string &type) : vklType(type)
  {
    ispcEquivalent    = ispc::Volume_createInstance_vklVolume(this);
    managedObjectType = OSP_VOLUME;
  }

  Volume::~Volume()
  {
    if (embreeGeometry)
      rtcReleaseGeometry(embreeGeometry);
  }

  std::string Volume::toString() const
  {
    return "ospray::Volume";
  }

  void Volume::commit()
  {
    if (vklVolume)
      vklRelease(vklVolume);

    vklVolume = vklNewVolume(vklType.c_str());

    if (!vklVolume)
      throw std::runtime_error("unsupported volume type '" + vklType + "'");

    handleParams();

    vklCommit(vklVolume);
    (vkl_box3f &)bounds = vklGetBoundingBox(vklVolume);

    createEmbreeGeometry();

    ispc::Volume_set(ispcEquivalent, embreeGeometry);
    ispc::Volume_set_vklVolume(
        ispcEquivalent, vklVolume, (ispc::box3f *)&bounds);
  }

  void Volume::createEmbreeGeometry()
  {
    if (embreeGeometry)
      rtcReleaseGeometry(embreeGeometry);

    embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
  }

  void Volume::handleParams()
  {
    // pass all supported parameters through to VKL volume object
    std::for_each(params_begin(), params_end(), [&](std::shared_ptr<Param> &p) {
      auto &param = *p;
      param.query = true;

      if (param.data.is<bool>()) {
        vklSetBool(vklVolume, param.name.c_str(), param.data.get<bool>());
      } else if (param.data.is<float>()) {
        vklSetFloat(vklVolume, param.name.c_str(), param.data.get<float>());
      } else if (param.data.is<int>()) {
        vklSetInt(vklVolume, param.name.c_str(), param.data.get<int>());
      } else if (param.data.is<vec3f>()) {
        vklSetVec3f(vklVolume,
                    param.name.c_str(),
                    param.data.get<vec3f>().x,
                    param.data.get<vec3f>().y,
                    param.data.get<vec3f>().z);
      } else if (param.data.is<void *>()) {
        vklSetVoidPtr(vklVolume, param.name.c_str(), param.data.get<void *>());
      } else if (param.data.is<const char *>()) {
        vklSetString(
            vklVolume, param.name.c_str(), param.data.get<const char *>());
      } else if (param.data.is<vec3i>()) {
        vklSetVec3i(vklVolume,
                    param.name.c_str(),
                    param.data.get<vec3i>().x,
                    param.data.get<vec3i>().y,
                    param.data.get<vec3i>().z);
      } else if (param.data.is<ManagedObject *>()) {
        Data *data           = (Data *)param.data.get<ManagedObject *>();

        if (data->type == OSP_DATA) {
          const Ref<const DataT<Data *>> blockData =
              getParamDataT<Data *>(param.name.c_str(), true);
          std::vector<VKLData> vklBlockData;
          for (auto &&data : *blockData) {
            VKLData vklData = vklNewData(data->size(),
                                         (VKLDataType)data->type,
                                         data->data(),
                                         VKL_DATA_SHARED_BUFFER);
            vklBlockData.push_back(vklData);
          }
          VKLData vklData =
              vklNewData(vklBlockData.size(), VKL_DATA, vklBlockData.data());
          vklSetData(vklVolume, param.name.c_str(), vklData);
          vklRelease(vklData);
        } else {
          VKLData vklData = vklNewData(data->size(),
                                       (VKLDataType)data->type,
                                       data->data(),
                                       VKL_DATA_SHARED_BUFFER);
          vklSetData(vklVolume, param.name.c_str(), vklData);
          vklRelease(vklData);
        }
      } else {
        param.query = false;
      }
    });
  }

  OSPTYPEFOR_DEFINITION(Volume *);

}  // namespace ospray
