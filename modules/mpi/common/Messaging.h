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

// mpiCommon
#include "mpiCommon/MPICommon.h"
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

    } // ::ospray::mpi::messaging
  } // ::ospray::mpi
} // ::ospray
