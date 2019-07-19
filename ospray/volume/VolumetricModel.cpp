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
#include "VolumetricModel.h"
#include "transferFunction/TransferFunction.h"
// ispc exports
#include "Volume_ispc.h"
#include "VolumetricModel_ispc.h"

namespace ospray {

  VolumetricModel::VolumetricModel(Volume *_volume)
  {
    if (_volume == nullptr)
      throw std::runtime_error("NULL Volume given to VolumetricModel!");

    volume = _volume;

    this->ispcEquivalent = ispc::VolumetricModel_create(this, volume->getIE());
  }

  std::string VolumetricModel::toString() const
  {
    return "ospray::VolumetricModel";
  }

  void VolumetricModel::commit()
  {
    auto *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error("no transfer function specified on the volume!");

    // Finish getting/setting other appearance information //

    volumeBounds = volume->bounds;

    ispc::VolumetricModel_set(ispcEquivalent,
                              getParam<float>("samplingRate", 0.125f),
                              transferFunction->getIE(),
                              (const ispc::box3f &)volumeBounds);
  }

  RTCGeometry VolumetricModel::embreeGeometryHandle() const
  {
    return volume->embreeGeometry;
  }

  box3f VolumetricModel::bounds() const
  {
    return volumeBounds;
  }

  void VolumetricModel::setGeomID(int geomID)
  {
    ispc::Volume_set_geomID(volume->getIE(), geomID);
  }

}  // namespace ospray
