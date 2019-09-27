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
#include "common/Util.h"
#include "transferFunction/TransferFunction.h"

namespace ospray {

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

  OSPTYPEFOR_DEFINITION(Volume *);

}  // namespace ospray
