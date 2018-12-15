// ======================================================================== //
// Copyright 2016-2018 Intel Corporation                                    //
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

#include "Context.h"
#include <iostream>

#include "ospcommon/memory/malloc.h"
#include "ospcommon/tasking/async.h"
#include "ospcommon/tasking/tasking_system_handle.h"
#include "ospcommon/utility/getEnvVar.h"

using ospcommon::AsyncLoop;
using ospcommon::make_unique;
using ospcommon::tasking::numTaskingThreads;
using ospcommon::utility::getEnvVar;

namespace maml {

  /*! the singleton object that handles all the communication */
  std::unique_ptr<Context> Context::singleton = make_unique<Context>();

  Context::~Context()
  {
    stop();
  }

  /*! register a new incoing-message handler. if any message comes in
    on the given communicator we'll call this handler */
  void Context::registerHandlerFor(MPI_Comm comm, MessageHandler *handler)
  {
    if (handlers.find(comm) != handlers.end()) {
      std::cerr << CODE_LOCATION
                << ": Warning: handler for this MPI_Comm already installed"
                << std::endl;
    }

    handlers[comm] = handler;

    /*! todo: to avoid race conditions we MAY want to check if there's
        any messages we've already received that would match this
        handler */
  }

  /*! put the given message in the outbox. note that this can be
    done even if the actual sending mechanism is currently
    stopped */
  void Context::send(std::shared_ptr<Message> msg)
  {
    outbox.push_back(msg);
  }

  void Context::processInboxMessages()
  {
    if (!inbox.empty()) {
      auto incomingMessages = inbox.consume();

      for (auto &message : incomingMessages) {
        auto *handler = handlers[message->comm];
        handler->incoming(message);
      }
    }
  }

  void Context::sendMessagesFromOutbox()
  {
    if (!outbox.empty()) {
      auto outgoingMessages = outbox.consume();

      for (auto &msg : outgoingMessages) {
        MPI_Request request;
        MPI_CALL(Isend(msg->data, msg->size, MPI_BYTE, msg->rank,
                       msg->tag, msg->comm, &request));
        pendingSends.push_back(request);
        sendCache.push_back(std::move(msg));
      }
    }
  }

  void Context::pollForAndRecieveMessages()
  {
    for (auto &it : handlers) {
      MPI_Comm comm = it.first;

      /* probe if there's something incoming on this handler's comm */
      int hasIncoming = 0;
      MPI_Status status;
      MPI_CALL(Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG,
                      comm, &hasIncoming, &status));

      if (hasIncoming) {
        int size;
        MPI_CALL(Get_count(&status, MPI_BYTE, &size));

        auto msg = std::make_shared<Message>(size);
        msg->rank = status.MPI_SOURCE;
        msg->tag  = status.MPI_TAG;
        msg->comm = comm;

        MPI_Request request;
        MPI_CALL(Irecv(msg->data, size, MPI_BYTE, msg->rank,
                       msg->tag, msg->comm, &request));
        pendingRecvs.push_back(request);
        recvCache.push_back(std::move(msg));
      }
    }
  }

  void Context::waitOnSomeSendRequests()
  {
    if (!pendingSends.empty()) {
      int numDone = 0;
      int *done = STACK_BUFFER(int, pendingSends.size());

      MPI_CALL(Testsome(pendingSends.size(), pendingSends.data(), &numDone,
                        done, MPI_STATUSES_IGNORE));

      for (int i = 0; i < numDone; ++i) {
        int pendingSendCompletedIndex = done[i];
        pendingSends[pendingSendCompletedIndex] = MPI_REQUEST_NULL;
        sendCache[pendingSendCompletedIndex].reset();
      }

      sendCache.erase(std::remove(sendCache.begin(),
                                  sendCache.end(),
                                  nullptr), sendCache.end());

      pendingSends.erase(std::remove(pendingSends.begin(),
                                     pendingSends.end(),
                                     MPI_REQUEST_NULL), pendingSends.end());
    }
  }

  void Context::waitOnSomeRecvRequests()
  {
    if (!pendingRecvs.empty()) {
      int numDone = 0;
      int *done = STACK_BUFFER(int, pendingRecvs.size());

      MPI_CALL(Waitsome(pendingRecvs.size(), pendingRecvs.data(), &numDone,
                        done, MPI_STATUSES_IGNORE));

      for (int i = 0; i < numDone; ++i) {
        int pendingRecvCompletedIndex = done[i];
        pendingRecvs[pendingRecvCompletedIndex] = MPI_REQUEST_NULL;
        inbox.push_back(std::move(recvCache[pendingRecvCompletedIndex]));
      }

      recvCache.erase(std::remove(recvCache.begin(),
                                  recvCache.end(),
                                  nullptr), recvCache.end());

      pendingRecvs.erase(std::remove(pendingRecvs.begin(),
                                     pendingRecvs.end(),
                                     MPI_REQUEST_NULL), pendingRecvs.end());
    }
  }

  void Context::flushRemainingMessages()
  {
    sendMessagesFromOutbox();

    while (!pendingRecvs.empty() && !pendingSends.empty() && !inbox.empty()) {
      waitOnSomeRecvRequests();
      waitOnSomeSendRequests();
      processInboxMessages();
    }
  }

  /*! start the service; from this point on maml is free to use MPI
    calls to send/receive messages; if your MPI library is not
    thread safe the app should _not_ do any MPI calls until 'stop()'
    has been called */
  void Context::start()
  {
    if (!isRunning()) {
      tasksAreRunning = true;

      auto launchMethod = AsyncLoop::LaunchMethod::AUTO;

      auto MAML_SPAWN_THREADS = getEnvVar<int>("MAML_SPAWN_THREADS");
      if (MAML_SPAWN_THREADS) {
        launchMethod = MAML_SPAWN_THREADS.value() ?
            AsyncLoop::LaunchMethod::THREAD : AsyncLoop::LaunchMethod::TASK;
      }

      if (!sendReceiveThread.get()) {
        sendReceiveThread = make_unique<AsyncLoop>([&](){
          sendMessagesFromOutbox();
          pollForAndRecieveMessages();

          waitOnSomeSendRequests();
          waitOnSomeRecvRequests();
        }, launchMethod);
      }

      if (!processInboxThread.get()) {
        processInboxThread = make_unique<AsyncLoop>([&](){
          processInboxMessages();
        }, launchMethod);
      }

      sendReceiveThread->start();
      processInboxThread->start();
    }
  }

  bool Context::isRunning() const
  {
    return tasksAreRunning;
  }

  /*! stops the maml layer; maml will no longer perform any MPI calls;
    if the mpi layer is not thread safe the app is then free to use
    MPI calls of its own, but it should not expect that this node
    receives any more messages (until the next 'start()' call) even
    if they are already in flight */
  void Context::stop()
  {
    tasksAreRunning = false;
    if (sendReceiveThread) {
      sendReceiveThread->stop();
    }
    if (processInboxThread) {
      processInboxThread->stop();
    }
    flushRemainingMessages();
  }

} // ::maml
