// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/MPIBcastFabric.h"
#include "common/MPICommon.h"
#include "common/ObjectHandle.h"
#include "maml/maml.h"

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
