// ======================================================================== //
// Copyright 2016-2019 Intel Corporation                                    //
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

#include "maml.h"
#include "Context.h"

namespace maml {

using ospcommon::make_unique;

// maml API definitions /////////////////////////////////////////////////////

void logMessageTimings(std::ostream &os)
{
  Context::singleton->logMessageTimings(os);
}

/*! start the service; from this point on maml is free to use MPI
    calls to send/receive messages; if your MPI library is not
    thread safe the app should _not_ do any MPI calls until 'stop()'
    has been called */
void init(bool enableCompression)
{
  Context::singleton = make_unique<Context>(enableCompression);
}

/*! stops the maml layer; maml will no longer perform any MPI calls;
    if the mpi layer is not thread safe the app is then free to use
    MPI calls of its own, but it should not expect that this node
    receives any more messages (until the next 'start()' call) even
    if they are already in flight */
void shutdown()
{
  Context::singleton = nullptr;
}

/*! register a new incoing-message handler. if any message comes in
  on the given communicator we'll call this handler */
void registerHandlerFor(MPI_Comm comm, MessageHandler *handler)
{
  Context::singleton->registerHandlerFor(comm, handler);
}

void start()
{
  Context::singleton->start();
}

bool isRunning()
{
  return Context::singleton && Context::singleton->isRunning();
}

void stop()
{
  Context::singleton->stop();
}

/*! send given message to given comm:rank. Once this function has
  called maml has full ownership of this message, and the user may
  no longer access it (because maml may delete it at any time) */
void sendTo(MPI_Comm comm, int rank, std::shared_ptr<Message> msg)
{
  if (!(rank >= 0 && msg.get()))
    OSPRAY_THROW("Incorrect argument values given to maml::sendTo(...)");

  msg->rank = rank;
  msg->comm = comm;
  Context::singleton->send(msg);
}

void queueCollective(std::shared_ptr<Collective> col)
{
  Context::singleton->queueCollective(col);
}

} // namespace maml
