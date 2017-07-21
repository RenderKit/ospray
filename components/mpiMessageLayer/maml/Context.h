// ************************************************************************** //
// Copyright 2016 Ingo Wald                                                   //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// You may obtain a copy of the License at                                    //
//                                                                            //
// http://www.apache.org/licenses/LICENSE-2.0                                 //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS,          //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
// ************************************************************************** //

#pragma once

#include "maml.h"
//ospcommon
#include "ospcommon/AsyncLoop.h"
#include "ospcommon/containers/TransactionalBuffer.h"
//stl
#include <future>
#include <map>
#include <mutex>
#include <vector>

namespace maml {

  /*! the singleton object that handles all the communication */
  struct OSPRAY_MAML_INTERFACE Context
  {
    Context() = default;
    ~Context();

    static std::unique_ptr<Context> singleton;

    /*! register a new incoing-message handler. if any message comes in
      on the given communicator we'll call this handler */
    void registerHandlerFor(MPI_Comm comm, MessageHandler *handler);

    /*! start the service; from this point on maml is free to use MPI
      calls to send/receive messages; if your MPI library is not
      thread safe the app should _not_ do any MPI calls until 'stop()'
      has been called */
    void start();

    bool isRunning() const;

    /*! stops the maml layer; maml will no longer perform any MPI calls;
      if the mpi layer is not thread safe the app is then free to use
      MPI calls of its own, but it should not expect that this node
      receives any more messages (until the next 'start()' call) even
      if they are already in flight */
    void stop();

    /*! put the given message in the outbox. note that this can be
        done even if the actual sending mechanism is currently
        stopped */
    void send(std::shared_ptr<Message> msg);

  private:

    // Helper functions //

    /*! the thread (function) that executes all MPI commands to
        send/receive messages via MPI.

        Some notes:

        - this thread does MPI calls (only!) between calls of start()
        and stop(). unless you call start(), nothing will ever get sent
        or received.

        - it only looks for incoming messages on communicators for
        which a handler has been specified. if you don't add a handler
        to a comm, nothing will ever get received from this comm (you
        may still send on it, though!)

        - messages to be sent are retrieved from 'outbox'; messages that
          are received get put to 'inbox' and the 'inboxcondition' gets
          triggered. it's another thread's job to execute those
          messages
    */
    void mpiSendAndRecieveTask();

    /*! the thread that executes messages that the receiver thread
        put into the inbox */
    void processInboxTask();

    void processInboxMessages();

    void sendMessagesFromOutbox();
    void pollForAndRecieveMessages();

    void waitOnSomeSendRequests();
    void waitOnSomeRecvRequests();

    void flushRemainingMessages();

    // Data members //

    bool tasksAreRunning {false};

    std::future<void> sendReceiveFuture;
    std::future<void> processInboxFuture;

    ospcommon::TransactionalBuffer<std::shared_ptr<Message>> inbox;
    ospcommon::TransactionalBuffer<std::shared_ptr<Message>> outbox;

    // NOTE(jda) - sendCache/pendingSends MUST correspond with each other by
    //             their index in their respective vectors...
    std::vector<std::shared_ptr<Message>> sendCache;
    std::vector<MPI_Request>              pendingSends;

    // NOTE(jda) - recvCache/pendingRecvs MUST correspond with each other by
    //             their index in their respective vectors...
    std::vector<std::shared_ptr<Message>> recvCache;
    std::vector<MPI_Request>              pendingRecvs;

    std::map<MPI_Comm, MessageHandler *> handlers;

    bool useTaskingSystem {true};

    // NOTE(jda) - these are only used when _not_ using the tasking sytem...
    std::unique_ptr<ospcommon::AsyncLoop> sendReceiveThread;
    std::unique_ptr<ospcommon::AsyncLoop> processInboxThread;
  };

} // ::maml
