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

  //////////////////////////////////////////////
  // helper for converting OSP to VKL data types
  std::unordered_map<OSPDataType, VKLDataType> OspToVklDataType =
  {
    { OSP_CHAR,   VKL_CHAR   },
    { OSP_UCHAR , VKL_UCHAR  },
    { OSP_VEC2UC, VKL_UCHAR2 },
    { OSP_VEC3UC, VKL_UCHAR3 },
    { OSP_VEC4UC, VKL_UCHAR4 },
    { OSP_SHORT , VKL_SHORT  },
    { OSP_USHORT, VKL_USHORT },
    { OSP_INT,    VKL_INT    },
    { OSP_VEC2I,  VKL_INT2   },
    { OSP_VEC3I,  VKL_INT3   },
    { OSP_VEC4I,  VKL_INT4   },
    { OSP_UINT,   VKL_UINT   },
    { OSP_VEC2UI, VKL_UINT2  },
    { OSP_VEC3UI, VKL_UINT3  },
    { OSP_VEC4UI, VKL_UINT4  },
    { OSP_LONG,   VKL_LONG   },
    { OSP_VEC2L,  VKL_LONG2  },
    { OSP_VEC3L,  VKL_LONG3  },
    { OSP_VEC4L,  VKL_LONG4  },
    { OSP_ULONG,  VKL_ULONG  },
    { OSP_VEC2UL, VKL_ULONG2 },
    { OSP_VEC3UL, VKL_ULONG3 },
    { OSP_VEC4UL, VKL_ULONG4 },
    { OSP_FLOAT,  VKL_FLOAT  },
    { OSP_VEC2F,  VKL_FLOAT2 },
    { OSP_VEC3F,  VKL_FLOAT3 },
    { OSP_VEC4F,  VKL_FLOAT4 },
    { OSP_DOUBLE, VKL_DOUBLE },
    { OSP_BOX1I,  VKL_BOX1I  },
    { OSP_BOX2I,  VKL_BOX2I  },
    { OSP_BOX3I,  VKL_BOX3I  },
    { OSP_BOX4I,  VKL_BOX4I  },
    { OSP_BOX1F,  VKL_BOX1F  },
    { OSP_BOX2F,  VKL_BOX2F  },
    { OSP_BOX3F,  VKL_BOX3F  },
    { OSP_BOX4F,  VKL_BOX4F  },
    { OSP_DATA,   VKL_DATA   }
  };

  inline VKLDataType getVKLDataType(OSPDataType ospDataType)
  {
    auto vklDataType = OspToVklDataType.find(ospDataType);
    if (vklDataType == OspToVklDataType.end()) {
      std::cerr << "unknown data type " << ospDataType << std::endl;
      return VKL_UNKNOWN;
    }
    return vklDataType->second;
  }
  //////////////////////////////////////////////

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

  inline VKLDataType getVKLDataType(OSPDataType dataType)
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
      defaut:
        std::cerr << "[Volume] unknown data type " << dataType << std::endl;
        return VKL_UNKNOWN;
    }
    return VKL_UNKNOWN;
  }

  void Volume::handleParams()
  {
    // pass all supported parameters through to VKL volume object
    std::for_each(params_begin(), params_end(), [&](std::shared_ptr<Param> &p)
    {
      auto &param = *p;

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
          std::vector<VKLData> blockData;
          for (size_t i = 0; i < data->size(); ++i)
          {
            Data* ospData = ((Data**)data->data())[i];
            VKLDataType vklDataType = getVKLDataType(ospData->type);
            VKLData vklData = vklNewData(ospData->size(),
                                         vklDataType,
                                         ospData->data(),
                                         VKL_DATA_SHARED_BUFFER);
            blockData.push_back(vklData);
          }
          VKLData vklData = vklNewData(blockData.size(),
                                       VKL_DATA,
                                       blockData.data(),
                                       VKL_DATA_SHARED_BUFFER);
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
        std::cerr << "[Volume] ignoring unsupported "
                     "parameter type: "
                  << param.name << std::endl;
      }
    });
  }

  OSPTYPEFOR_DEFINITION(Volume *);

}  // namespace ospray
