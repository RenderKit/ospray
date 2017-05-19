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

#pragma once

#include "ospcommon/networking/BufferedDataStreaming.h"
// mpiCommon
#include "mpiCommon/MPICommon.h"
#include "mpiCommon/MPIBcastFabric.h"
// maml
#include "maml/maml.h"
// ospray
#include "ospray/common/ObjectHandle.h"

namespace ospray {
  namespace mpi {
    namespace messaging {

      // async point messaging interface //////////////////////////////////////

      void registerMessageListener(int handleObjID,
                                   maml::MessageHandler *listener);

      void enableAsyncMessaging();

      bool asyncMessagingEnabled();

      void sendTo(int globalRank, ObjectHandle object,
                  std::shared_ptr<mpicommon::Message> msg);

      void disableAsyncMessaging();

      // collective messaging interface //////////////////////////////////////
      // Broadcast some data, if rank == rootGlobalRank we send data, otherwise
      // the received data will be put in data.
      // TODO: Handling non-world groups? I'm not sure how
      // we could set up object handlers for the bcast or collective layer,
      // since it's inherently a global sync operation. If we async dispatched
      // collectives then maybe? But how could you ensure the queue of
      // collectives seen by all nodes was the same order? You might
      // mismatch collectives or hang trying to dispatch
      // the wrong ones?
      template<typename T>
      void bcast(const int rootGlobalRank, std::vector<T> &data) {
        const bool asyncWasRunning = asyncMessagingEnabled();
        disableAsyncMessaging();

        // TODO: We don't want to re-use the MPIOffload fabric because it's
        // an intercommunicator between the app/worker groups and thus will
        // only support bcast from master -> workers, not the workers <-> workers
        // style communication we want here. Is it best to just create
        // a fabric each time? Or have some other fabric we save statically?
        // The distributed device doesn't have a fabric in it by default either.
        mpicommon::MPIBcastFabric fabric(mpicommon::world, rootGlobalRank);
        if (mpicommon::globalRank() == rootGlobalRank) {
          networking::BufferedWriteStream stream(fabric);
          stream << data;
          stream.flush();
        } else {
          networking::BufferedReadStream stream(fabric);
          stream >> data;
        }

        if (asyncWasRunning) {
          std::cout << "Async was running, re-enabling\n";
          enableAsyncMessaging();
        }
      }

    } // ::ospray::mpi::messaging
  } // ::ospray::mpi
} // ::ospray
