// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "api/Device.h"
#include "DistributedModel.h"
#include "mpiCommon/MPICommon.h"
#include "Messaging.h"
// ispc exports
#include "DistributedModel_ispc.h"

namespace ospray {
  namespace mpi {

    extern "C" void *ospray_getEmbreeDevice()
    {
      return api::Device::current->embreeDevice;
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
      // TODO: We may need to override the ISPC calls made
      // to the Model or customize the model struct on the ISPC
      // side. In which case we need some ISPC side inheritence
      // for the model type. Currently the code is actually identical.
      Model::commit();
      //TODO: send my bounding boxes to other nodes, recieve theirs for a
      //      "full picture" of what geometries live on what nodes
      std::vector<int> result = {mpicommon::globalRank()};
      messaging::bcast(0, result);
      std::cout << "Rank " << mpicommon::globalRank() << ": Got back "
        << result[0] << "\n";
    }

  } // ::ospray::mpi
} // ::ospray
