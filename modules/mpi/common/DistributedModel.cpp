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

#include <iterator>
#include <algorithm>
// ospray
#include "api/ISPCDevice.h"
#include "DistributedModel.h"
#include "mpiCommon/MPICommon.h"
#include "Messaging.h"
#include "common/Data.h"
// ispc exports
#include "DistributedModel_ispc.h"

namespace ospray {
  namespace mpi {

    extern "C" void *ospray_getEmbreeDevice()
    {
      return api::ISPCDevice::embreeDevice;
    }

    DistributedModel::DistributedModel()
    {
      managedObjectType = OSP_MODEL;
      this->ispcEquivalent = ispc::DistributedModel_create(this);
    }

    std::string DistributedModel::toString() const
    {
      return "ospray::mpi::DistributedModel";
    }

    void DistributedModel::commit()
    {
      Model::commit();
      id = getParam1i("id", -1);
      if (id == -1) {
        throw std::runtime_error("#osp:error An id must be set for the model");
      }
      // TODO: We should take an optional clipping box to shrink the
      // model's bounds, for example if we want to clip some sphere
      // or other geometry to be smaller and contained within the actual
      // box we own.
      if (hasParam("region.lower") && hasParam("region.upper")) {
        bounds = box3f(getParam<vec3f>("region.lower", bounds.lower),
                       getParam<vec3f>("region.upper", bounds.upper));
        ispc::DistributedModel_setBounds(getIE(), (ispc::box3f*)&bounds);
      }
    }

  } // ::ospray::mpi
} // ::ospray

