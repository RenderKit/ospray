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

#include <iterator>
#include <algorithm>
// ospray
#include "api/Device.h"
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
      othersRegions.clear();

      // TODO: We may need to override the ISPC calls made
      // to the Model or customize the model struct on the ISPC
      // side. In which case we need some ISPC side inheritence
      // for the model type. Currently the code is actually identical.
      Model::commit();
      // Send my bounding boxes to other nodes, recieve theirs for a
      // "full picture" of what geometries live on what nodes
      Data *regionData = getParamData("regions");

      // The box3f data is sent as data of FLOAT3 items
      // TODO: It's a little awkward to copy the boxes again like this, maybe
      // can re-thinkg the send side of the bcast call? One that takes
      // a ptr and a size since we know we won't be writing out to it?
      // TODO: For now it doesn't matter that we don't know who owns the
      // other boxes, just that we know they exist and their bounds, and that
      // they aren't ours.
      if (regionData) {
        box3f *boxes = reinterpret_cast<box3f*>(regionData->data);
        myRegions = std::vector<box3f>(boxes, boxes + regionData->numItems / 2);
      }

      // If the user hasn't set any regions, there's an implicit infinitely
      // large region box we can place around the entire world.
      if (myRegions.empty()) {
        postStatusMsg("No regions found, making implicit "
                      "infinitely large region", 1);
        myRegions.push_back(box3f(vec3f(neg_inf), vec3f(pos_inf)));
      }

      for (int i = 0; i < mpicommon::numGlobalRanks(); ++i) {
        if (i == mpicommon::globalRank()) {
          messaging::bcast(i, myRegions);
        } else {
          std::vector<box3f> recv;
          messaging::bcast(i, recv);
          std::copy(recv.begin(), recv.end(),
                    std::back_inserter(othersRegions));
        }
      }

      if (logLevel() >= 1) {
        for (int i = 0; i < mpicommon::numGlobalRanks(); ++i) {
          if (i == mpicommon::globalRank()) {
            postStatusMsg(1) << "Rank " << mpicommon::globalRank()
              << ": Got regions from others {";
            for (const auto &b : othersRegions) {
              postStatusMsg(1) << "\t" << b << ",";
            }
            postStatusMsg(1) << "}";
          }
        }
      }
    }

  } // ::ospray::mpi
} // ::ospray
