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

#pragma once

#include "common/MPIBcastFabric.h"
#include "common/MPICommon.h"
#include "maml/maml.h"
#include "ospray/common/ObjectHandle.h"

namespace ospray {
namespace mpi {
namespace messaging {

// message handling base ////////////////////////////////////////////////

struct MessageHandler : public maml::MessageHandler
{
  //! NOTE: automatically register/de-registers itself

  MessageHandler(ObjectHandle handle);
  virtual ~MessageHandler();

 protected:
  ObjectHandle myId;
};

// async point messaging interface //////////////////////////////////////

/* Register the object message handler and dispatcher to run all object
 * fire & forget messaging on the specified group. The internal
 * communicator used will be a dup'd from this group, to avoid
 * conflicting with other messaging.
 */
void init(mpicommon::Group parentGroup);

void registerMessageListener(int handleObjID, MessageHandler *listener);

void removeMessageListener(int handleObjID);

void enableAsyncMessaging();

bool asyncMessagingEnabled();

void sendTo(int globalRank,
    ObjectHandle object,
    std::shared_ptr<mpicommon::Message> msg);

void disableAsyncMessaging();

} // namespace messaging
} // namespace mpi
} // namespace ospray
