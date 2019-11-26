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

  // Helper for converting OSP to VKL data types //////////////////////////////

  static VKLDataType getVKLDataType(OSPDataType dataType)
  {
    switch (dataType)
    {
      case OSP_CHAR:   return VKL_CHAR;
      case OSP_UCHAR:  return VKL_UCHAR;
      case OSP_VEC2UC: return VKL_UCHAR2;
      case OSP_VEC3UC: return VKL_UCHAR3;
      case OSP_VEC4UC: return VKL_UCHAR4;
      case OSP_SHORT:  return VKL_SHORT;
      case OSP_USHORT: return VKL_USHORT;
      case OSP_INT:    return VKL_INT;
      case OSP_VEC2I:  return VKL_INT2;
      case OSP_VEC3I:  return VKL_INT3;
      case OSP_VEC4I:  return VKL_INT4;
      case OSP_UINT:   return VKL_UINT;
      case OSP_VEC2UI: return VKL_UINT2;
      case OSP_VEC3UI: return VKL_UINT3;
      case OSP_VEC4UI: return VKL_UINT4;
      case OSP_LONG:   return VKL_LONG;
      case OSP_VEC2L:  return VKL_LONG2;
      case OSP_VEC3L:  return VKL_LONG3;
      case OSP_VEC4L:  return VKL_LONG4;
      case OSP_ULONG:  return VKL_ULONG;
      case OSP_VEC2UL: return VKL_ULONG2;
      case OSP_VEC3UL: return VKL_ULONG3;
      case OSP_VEC4UL: return VKL_ULONG4;
      case OSP_FLOAT:  return VKL_FLOAT;
      case OSP_VEC2F:  return VKL_FLOAT2;
      case OSP_VEC3F:  return VKL_FLOAT3;
      case OSP_VEC4F:  return VKL_FLOAT4;
      case OSP_DOUBLE: return VKL_DOUBLE;
      case OSP_BOX1I:  return VKL_BOX1I;
      case OSP_BOX2I:  return VKL_BOX2I;
      case OSP_BOX3I:  return VKL_BOX3I;
      case OSP_BOX4I:  return VKL_BOX4I;
      case OSP_BOX1F:  return VKL_BOX1F;
      case OSP_BOX2F:  return VKL_BOX2F;
      case OSP_BOX3F:  return VKL_BOX3F;
      case OSP_BOX4F:  return VKL_BOX4F;
      case OSP_DATA:   return VKL_DATA;
      default:
        std::cerr << "[Volume] unknown data type " << dataType << std::endl;
        return VKL_UNKNOWN;
    }
  }

  // Volume defintions ////////////////////////////////////////////////////////

  Volume::Volume()
  {
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

  Volume *Volume::createInstance(const std::string &type)
  {
    return createInstanceHelper<Volume, OSP_VOLUME>(type);
  }

  void Volume::commit()
  {
    createEmbreeGeometry();

    ispc::Volume_set(ispcEquivalent, embreeGeometry);
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
    std::for_each(params_begin(), params_end(), [&](std::shared_ptr<Param> &p)
    {
      auto &param = *p;
      param.query = true;

      if (param.data.is<bool>()) {
        vklSetBool(vklVolume,
                   param.name.c_str(),
                   param.data.get<bool>());
      } else if (param.data.is<float>()) {
        vklSetFloat(vklVolume,
                    param.name.c_str(),
                    param.data.get<float>());
      } else if (param.data.is<int>()) {
        vklSetInt(vklVolume,
                    param.name.c_str(),
                    param.data.get<int>());
      } else if (param.data.is<vec3f>()) {
        vklSetVec3f(vklVolume,
                    param.name.c_str(),
                    param.data.get<vec3f>().x,
                    param.data.get<vec3f>().y,
                    param.data.get<vec3f>().z);
      } else if (param.data.is<void*>()) {
        vklSetVoidPtr(vklVolume,
                      param.name.c_str(),
                      param.data.get<void*>());
      } else if (param.data.is<const char*>()) {
        vklSetString(vklVolume,
                     param.name.c_str(),
                     param.data.get<const char*>());
      } else if (param.data.is<vec3i>()) {
        vklSetVec3i(vklVolume,
                    param.name.c_str(),
                    param.data.get<vec3i>().x,
                    param.data.get<vec3i>().y,
                    param.data.get<vec3i>().z);
      } else if (param.data.is<ManagedObject *>()) {
        Data *data = (Data *)param.data.get<ManagedObject *>();
        VKLDataType dataType = getVKLDataType(data->type);
        if (dataType == VKL_UNKNOWN) {
          std::cerr << "[Volume] unknown type for parameter "
                    << param.name << std::endl;
          return;
        }

        if (dataType == VKL_DATA) {
          const DataT<Data*>* blockData = getParamDataT<Data*>(param.name.c_str(), true);
          std::vector<VKLData> vklBlockData;
          for (auto iter = blockData->begin(); iter != blockData->end(); ++iter)
          {
            const Data* data = *iter;
            VKLData vklData = vklNewData(data->size(),
                                        getVKLDataType(data->type),
                                        data->data(),
                                        VKL_DATA_SHARED_BUFFER);
            vklBlockData.push_back(vklData);
          }
          VKLData vklData = vklNewData(vklBlockData.size(),
                                       VKL_DATA,
                                       vklBlockData.data());
          vklSetData(vklVolume, param.name.c_str(), vklData);
          vklRelease(vklData);
        }
        else
        {
          VKLData vklData = vklNewData(data->size(),
                                      dataType,
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
