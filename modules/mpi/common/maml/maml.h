// Copyright 2016-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ostream>
#include "common/Collectives.h"
#include "common/MPICommon.h"

namespace maml {

using Message = mpicommon::Message;
// Only bcast for now, to test
using Collective = mpicommon::Collective;

/*! abstraction for an object that can receive messages. handlers
    get associated with MPI_Comm's, and get called automatically
    every time such a message comes in. maml receives the message,
    then passed it to the handler, from which point it is owned by -
    and the respsonsibility of - the handler */
struct MessageHandler
{
  virtual void incoming(const std::shared_ptr<Message> &message) = 0;

  virtual ~MessageHandler() = default;
};

// WILL: Statistics logging
void logMessageTimings(std::ostream &os);

/*! initialize the service for this process and
    start the service; from this point on maml is free to use MPI
    calls to send/receive messages; if your MPI library is not
    thread safe the app should _not_ do any MPI calls until 'shutdown()'
    has been called */
void init(bool enableCompression = false);

/*! shutdown the service for this process.
    stops the maml layer; maml will no longer perform any MPI calls;
    if the mpi layer is not thread safe the app is then free to use
    MPI calls of its own, but it should not expect that this node
    receives any more messages (until the next 'init()' call) even
    if they are already in flight */
void shutdown();

/*! register a new incoing-message handler. if any message comes in
    on the given communicator we'll call this handler */
void registerHandlerFor(MPI_Comm comm, MessageHandler *handler);

/*! start the service; from this point on maml is free to use MPI
    calls to send/receive messages; if your MPI library is not
    thread safe the app should _not_ do any MPI calls until 'stop()'
    has been called */
void start();

bool isRunning();

/*! stops the maml layer; maml will no longer perform any MPI calls;
    if the mpi layer is not thread safe the app is then free to use
    MPI calls of its own, but it should not expect that this node
    receives any more messages (until the next 'start()' call) even
    if they are already in flight */
void stop();

/*! schedule the given message to be send to the given
    comm:rank. comm and rank have to be a valid address. Once this
    function has been called maml has full ownership of this message,
    and the user may no longer access it (because maml may delete it at
    any time). note this message will not be sent immediately if the
    mpi sending is stopped; it will, however, be placed in the
    outbox to be sent at the next possible opportunity.
    If a rank sends a message to itself the MPI communication
    layer will be skipped, however some cost may be incurred
    when compressing/decompressing the message and accessing
    the various inbox/outbox vectors.


    WARNING: calling flush does NOT guarantee that there's no more
    messages coming in to this node: it does mean that all the
    messages we've already MPI_Probe'd will get finished receiving,
    but it will NOT guarantee that another node(s) aren't just
    sending out some new message(s) that simply haven't even STARTED
    arriving on this node, yet!!!
*/
void sendTo(MPI_Comm comm, int rank, std::shared_ptr<Message> msg);

/*! Schedule a collective to be run on the messaging layer */
void queueCollective(std::shared_ptr<Collective> col);

} // namespace maml
