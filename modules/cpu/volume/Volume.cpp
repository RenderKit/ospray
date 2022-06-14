// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "volume/Volume.h"
#include "common/Data.h"
#include "common/Util.h"
#include "volume/Volume_ispc.h"

#include "openvkl/openvkl.h"
#include "openvkl/vdb.h"

#include <unordered_map>

namespace ospray {

// Volume definitions ////////////////////////////////////////////////////////

Volume::Volume(const std::string &type) : vklType(type)
{
  // check VKL has default config for VDB
  if (type == "vdb"
      && (vklVdbLevelNumVoxels(0) != 262144 || vklVdbLevelNumVoxels(1) != 32768
          || vklVdbLevelNumVoxels(2) != 4096 || vklVdbLevelNumVoxels(3) != 512
          || vklVdbLevelNumVoxels(4) != 0))
    throw std::runtime_error(toString()
        + " Open VKL has non-default configuration for VDB volumes.");

  managedObjectType = OSP_VOLUME;
}

Volume::~Volume()
{
  if (vklSampler)
    vklRelease(vklSampler);

  if (vklVolume)
    vklRelease(vklVolume);

  if (embreeGeometry)
    rtcReleaseGeometry(embreeGeometry);
}

std::string Volume::toString() const
{
  return "ospray::Volume";
}

void Volume::commit()
{
  if (!vklDevice) {
    throw std::runtime_error("invalid Open VKL device");
  }
  if (!embreeDevice) {
    throw std::runtime_error("invalid Embree device");
  }

  if (vklSampler)
    vklRelease(vklSampler);

  if (vklVolume)
    vklRelease(vklVolume);

  vklVolume = vklNewVolume(vklDevice, vklType.c_str());

  if (!vklVolume)
    throw std::runtime_error("unsupported volume type '" + vklType + "'");

  if (!embreeGeometry) {
    embreeGeometry = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_USER);
  }

  handleParams();

  vklCommit(vklVolume);
  (vkl_box3f &)bounds = vklGetBoundingBox(vklVolume);

  vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  // Setup Embree user-defined geometry
  rtcSetGeometryUserData(embreeGeometry, getSh());
  rtcSetGeometryUserPrimitiveCount(embreeGeometry, 1);
  rtcSetGeometryBoundsFunction(
      embreeGeometry, (RTCBoundsFunction)&ispc::Volume_embreeBounds, getSh());
  rtcSetGeometryIntersectFunction(
      embreeGeometry, (RTCIntersectFunctionN)&ispc::Volume_intersect_kernel);
  rtcCommitGeometry(embreeGeometry);

  // Initialize shared structure
  getSh()->vklVolume = vklVolume;
  getSh()->vklSampler = vklSampler;
  getSh()->boundingBox = bounds;
}

void Volume::checkDataStride(const Data *data) const
{
  if (data->stride().y != int64_t(data->numItems.x) * data->stride().x
      || data->stride().z != int64_t(data->numItems.y) * data->stride().y) {
    throw std::runtime_error(
        toString() + " Open VKL only supports 1D strides between elements");
  }
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
      Data *data = (Data *)param.data.get<ManagedObject *>();

      if (data->type == OSP_DATA) {
        auto &dataD = data->as<Data *>();
        std::vector<VKLData> vklBlockData;
        vklBlockData.reserve(data->size());
        for (auto &&data : dataD) {
          checkDataStride(data);
          VKLData vklData = vklNewData(vklDevice,
              data->size(),
              (VKLDataType)data->type,
              data->data(),
              VKL_DATA_SHARED_BUFFER,
              data->stride().x);
          vklBlockData.push_back(vklData);
        }
        VKLData vklData = vklNewData(
            vklDevice, vklBlockData.size(), VKL_DATA, vklBlockData.data());
        vklSetData(vklVolume, param.name.c_str(), vklData);
        vklRelease(vklData);
        for (VKLData vd : vklBlockData)
          vklRelease(vd);

        if (vklType == "vdb" && param.name == "node.data") {
          // deduce format
          std::vector<uint32_t> format;
          format.reserve(data->size());
          for (auto &&data : dataD) {
            bool isTile = data->size() == 1;
            if (!isTile) {
              if (data->numItems.x != data->numItems.y
                  || data->numItems.x != data->numItems.z)
                throw std::runtime_error(
                    toString() + " VDB leaf node data must have size n^3.");
            }
            format.push_back(isTile ? VKL_FORMAT_TILE : VKL_FORMAT_DENSE_ZYX);
          }
          VKLData vklData =
              vklNewData(vklDevice, format.size(), VKL_UINT, format.data());
          vklSetData(vklVolume, "node.format", vklData);
          vklRelease(vklData);
        }

      } else {
        checkDataStride(data);
        VKLData vklData = vklNewData(vklDevice,
            data->size(),
            (VKLDataType)data->type,
            data->data(),
            VKL_DATA_SHARED_BUFFER,
            data->stride().x);

        std::string name(param.name);
        if (name == "data") { // structured volumes
          vec3ul &dim = data->numItems;
          vklSetVec3i(vklVolume, "dimensions", dim.x, dim.y, dim.z);
          vklSetInt(vklVolume, "voxelType", (VKLDataType)data->type);
        }
        if (name == "nodesPackedDense" || name == "nodesPackedTile") {
          // packed VDB volumes: wrap attribute
          VKLData vklDataWrapper = vklNewData(vklDevice, 1, VKL_DATA, &vklData);
          vklRelease(vklData);
          vklData = vklDataWrapper;
        }
        vklSetData(vklVolume, name.c_str(), vklData);
        vklRelease(vklData);
      }
    } else {
      param.query = false;
    }
  });
}

void Volume::setDevice(RTCDevice embreed, VKLDevice vkld)
{
  embreeDevice = embreed;
  vklDevice = vkld;
}

OSPTYPEFOR_DEFINITION(Volume *);

} // namespace ospray
